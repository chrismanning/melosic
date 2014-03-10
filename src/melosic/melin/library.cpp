/**************************************************************************
**  Copyright (C) 2013 Christian Manning
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <mutex>
#include <atomic>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/variant/get.hpp>
#include <boost/functional/hash/hash.hpp>

#ifndef NDEBUG
#include <jbson/json_writer.hpp>
#endif
#include <jbson/json_reader.hpp>
#include <jbson/path.hpp>
using namespace jbson::literal;

#include <melosic/melin/config.hpp>
#include <melosic/common/directories.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/audiofile.hpp>
#include <melosic/common/optional.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/plugin.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/melin/input.hpp>
#include <melosic/common/typeid.hpp>
#include "library.hpp"

namespace Melosic {
namespace Library {

using std::mutex;
using unique_lock = std::unique_lock<mutex>;

struct ScanStarted : Signals::Signal<Signals::Library::ScanStarted> {};
struct ScanEnded : Signals::Signal<Signals::Library::ScanEnded> {};

static const fs::path DataDir{Directories::dataHome() / "melosic"};

struct PathEquivalence : std::binary_function<const fs::path&, const fs::path&, bool> {
    bool operator()(const fs::path& a, const fs::path& b) const { return fs::equivalent(a, b); }
};

struct Manager::impl {
    impl(Config::Manager&, Decoder::Manager&);

    void loadedSlot(boost::synchronized_value<Config::Conf>& ubase);
    void variableUpdateSlot(const Config::KeyType& key, const Config::VarType& val);

    void scan();
    void scan(const fs::path& dir);

    void incremental_scan();
    void incremental_scan(const fs::path& dir);

    Decoder::Manager& decman;
    Logger::Logger logject{logging::keywords::channel = "Library::Manager"};
    Config::Conf conf{"Library"};
    ejdb::ejdb db;
    ScanStarted scanStarted;
    ScanEnded scanEnded;
    boost::synchronized_value<Manager::SetType> m_dirs;
    mutex mu;
    std::atomic<bool> pluginsLoaded{false};
};

Manager::impl::impl(Config::Manager& confman, Decoder::Manager& decman) : decman(decman) {
    if(!fs::exists(DataDir))
        fs::create_directory(DataDir);
    assert(fs::exists(DataDir) && fs::is_directory(DataDir));
    std::error_code ec;
    db.open((DataDir / "medialibrary").c_str(),
            ejdb::db_mode::read | ejdb::db_mode::write | ejdb::db_mode::create | ejdb::db_mode::trans_sync, ec);

    //    TRACE_LOG(logject) << "db metadata:" << db.metadata();

    confman.getLoadedSignal().connect(&impl::loadedSlot, this);
}

void Manager::impl::loadedSlot(boost::synchronized_value<Config::Conf>& ubase) {
    //    unique_lock l(mu);
    TRACE_LOG(logject) << "Library conf loaded";

    auto base = ubase.synchronize();
    auto c = base->getChild("Library");
    if(!c) {
        base->putChild(conf);
        c = base->getChild("Library");
    }
    assert(c);
    c->merge(conf);
    c->addDefaultFunc([=]() { return conf; });
    c->iterateNodes([&](const std::tuple<Config::KeyType, Config::VarType>& pair) {
        using std::get;
        //        scope_unlock_exit_lock<decltype(l)> rl{l};
        TRACE_LOG(logject) << "Config: variable loaded: " << get<0>(pair);
        variableUpdateSlot(get<0>(pair), get<1>(pair));
    });
    c->getVariableUpdatedSignal().connect(&impl::variableUpdateSlot, this);
}

void Manager::impl::variableUpdateSlot(const Config::KeyType& key, const Config::VarType& val) {
    using std::get;
    //    unique_lock l(mu);
    TRACE_LOG(logject) << "Config: variable updated: " << key;
    try {
        if(key == "directories") {
            auto tmp_dirs = get<std::vector<Config::VarType>>(val);
            scanStarted();
            auto dirs = m_dirs.synchronize();
            for(auto&& dir : tmp_dirs) {
                auto p = get<std::string>(dir);
                if(!fs::is_directory(p))
                    continue;
                auto ret = dirs->insert(std::move(fs::canonical(p)));
                if(get<1>(ret))
                    scan(p);
            }
            scanEnded();
        }
    }
    catch(boost::bad_get&) {
        ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
    }
}

void Manager::impl::scan() {
    if(!pluginsLoaded)
        return;
    scanStarted();
    m_dirs([this](auto&& dirs) {
        for(auto&& dir : dirs) {
            scan(dir);
        }
    });
    scanEnded();
}

void Manager::impl::scan(const fs::path& dir) {
    if(!pluginsLoaded)
        return;
    assert(fs::exists(dir) && fs::is_directory(dir));
    TRACE_LOG(logject) << "scanning " << dir;
    std::error_code ec;
    auto coll = db.create_collection("tracks", ec);

    // remove all tracks in current prefix
    auto qry = db.create_query(R"({"$dropall":true})"_json_doc, ec);
    if(!qry || ec) {
        ERROR_LOG(logject) << "query creation error: " << ec.message();
        return;
    }
    auto res = coll.execute_query(qry, ejdb::query::count_only);
    assert(res.empty());
    auto r = coll.sync(ec);
    if(!r || ec) {
        ERROR_LOG(logject) << "collection sync error: " << ec.message();
        return;
    }

    for(fs::directory_entry& entry : fs::recursive_directory_iterator{dir}) {
        auto p = entry.path();
        if(!fs::is_regular_file(p))
            continue;
        TRACE_LOG(logject) << "Scanning: " << p;
        try {
            auto tracks = decman.tracks(Input::to_uri(p));
            if(tracks.empty())
                continue;
            for(const auto& t : tracks) {
                auto oid = coll.save_document(t.bson(), ec);
                if(ec) {
                    ERROR_LOG(logject) << "document save error: " << ec.message();
                    continue;
                }
                assert(oid);
            }
        }
        catch(...) {
            ERROR_LOG(logject) << "Scan error";
            ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
        }
    }
    r = coll.sync(ec);
    if(!r || ec) {
        ERROR_LOG(logject) << "collection sync error: " << ec.message();
        return;
    }
    r = db.sync(ec);
    if(!r || ec) {
        ERROR_LOG(logject) << "db sync error: " << ec.message();
        return;
    }
    TRACE_LOG(logject) << dir << " scanned";

#ifndef NDEBUG
    qry = db.create_query("{}"_json_doc, ec);
    if(!qry || ec) {
        ERROR_LOG(logject) << "query creation error: " << ec.message();
        return;
    }

    for(auto&& doc : coll.execute_query(qry)) {
        auto r = coll.sync(ec);
        if(!r || ec) {
            ERROR_LOG(logject) << "collection sync error: " << ec.message();
            return;
        }
        auto doc_str = std::string{};
        write_json(doc, std::back_inserter(doc_str));
        DEBUG_LOG(logject) << "track: " << doc_str;
    }
#endif
}

void Manager::impl::incremental_scan() {
    if(!pluginsLoaded)
        return;
    m_dirs([this](auto&& dirs) {
        for(auto&& dir : dirs) {
            incremental_scan(dir);
        }
    });
}

void Manager::impl::incremental_scan(const fs::path& dir) {
    if(!pluginsLoaded)
        return;
}

Manager::Manager(Config::Manager& confman, Decoder::Manager& decman, Plugin::Manager& plugman)
    : pimpl(new impl(confman, decman)) {
    plugman.getPluginsLoadedSignal().connect([this](auto&&) {
        pimpl->pluginsLoaded.store(true);
        pimpl->scan();
    });
}

Manager::~Manager() {}

const boost::synchronized_value<Manager::SetType>& Manager::getDirectories() const { return pimpl->m_dirs; }

void Manager::scan() {
    //    pimpl->scanStarted();
    //    unique_lock l(pimpl->mu);
    //    l.unlock();
    //    pimpl->scanEnded();
}

ejdb::ejdb& Manager::getDataBase() const { return pimpl->db; }

std::vector<jbson::document> Manager::query(const jbson::document& qdoc) const {
    std::error_code ec;
    ejdb::query q;
    try {
        q = pimpl->db.create_query(qdoc, ec).set_hints(R"({ "$orderby": { "location": 1 } })"_json_doc);
        if(ec)
            return {};
    }
    catch(...) {
        return {};
    }

    auto coll = pimpl->db.create_collection("tracks", ec);
    if(ec)
        return {};

    return coll.execute_query(q);
}

std::vector<jbson::element> Manager::query(const jbson::document& q, boost::string_ref json_path) const {
    std::vector<jbson::element> vec;
    auto docs = query(q);
    if(json_path.empty() || (json_path.size() == 1 && json_path.front() == '$')) {
        for(auto&& doc : docs)
            for(auto&& e : doc)
                vec.emplace_back(e);
        return std::move(vec);
    }
    for(auto&& doc : docs)
        for(auto&& e : jbson::path_select(doc, json_path))
            vec.emplace_back(e);
    return std::move(vec);
}

std::vector<jbson::document_set>
Manager::query(const jbson::document& q,
               ForwardRange<const std::tuple<std::string, std::string>> paths) const {
    std::vector<jbson::document_set> res;
    for(auto&& doc : query(q)) {
        jbson::document_set set;
        for(auto&& named_path : paths) {
            for(auto&& e : jbson::path_select(doc, std::get<1>(named_path))) {
                e.name(std::get<0>(named_path));
                assert(e.name() == std::get<0>(named_path));
                set.emplace(e);
            }
        }
        res.emplace_back(std::move(set));
    }
    return std::move(res);
}

std::vector<jbson::document_set>
Manager::query(const jbson::document& q,
               std::initializer_list<std::tuple<std::string, std::string>> paths) const {
    const auto p = ForwardRange<const std::tuple<std::string, std::string>>(paths);
    return query(q, p);
}

Signals::Library::ScanStarted& Manager::getScanStartedSignal() noexcept { return pimpl->scanStarted; }

Signals::Library::ScanEnded& Manager::getScanEndedSignal() noexcept { return pimpl->scanEnded; }

} // namespace Library
} // namespace Melosic
