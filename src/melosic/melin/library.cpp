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
#include <boost/functional/hash.hpp>

#include <ejpp/ejdb.hpp>
#include <bson/bson.h>

#include <melosic/melin/config.hpp>
#include <melosic/common/directories.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/audiofile.hpp>
#include <melosic/common/optional.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/plugin.hpp>
#include <melosic/melin/decoder.hpp>
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

    optional<Core::AudioFile> getFile(const fs::path& p_, std::error_code& ec) {
        auto file_coll = db.create_collection("files", ec);
        if(ec)
            return nullopt;

        fs::path p{fs::canonical(p_ /*, ec*/)};
        if(!fs::exists(p) || fs::is_other(p) || fs::is_directory(p)) {
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
            return nullopt;
        }
        ejdb::query q = db.create_query(BSON("path" << p.generic_string()), ec);
        if(ec)
            return nullopt;
        auto results = file_coll.execute_query(q);
        if(results.empty())
            return nullopt;

        assert(results.size() == 1);
        auto file_obj = results.front();

        Core::AudioFile af;
        file_obj >> af;
        if(!af.pimpl)
            return nullopt;

        return af;
    }

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

    TRACE_LOG(logject) << "db metadata:" << db.metadata();

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
            auto dirs = m_dirs.synchronize();
            for(auto&& dir : tmp_dirs) {
                auto p = get<std::string>(dir);
                if(!fs::is_directory(p))
                    continue;
                auto ret = dirs->insert(std::move(fs::canonical(p)));
                if(get<1>(ret))
                    scan(p);
            }
        }
    }
    catch(boost::bad_get&) {
        ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
    }
}

void Manager::impl::scan() {
    if(!pluginsLoaded)
        return;
    m_dirs([this](auto&& dirs) {
        for(auto&& dir : dirs) {
            scan(dir);
        }
    });
}

void Manager::impl::scan(const fs::path& dir) {
    if(!pluginsLoaded)
        return;
    assert(fs::exists(dir) && fs::is_directory(dir));
    TRACE_LOG(logject) << "scanning " << dir;
    std::error_code ec;
    auto coll = db.create_collection("tracks", ec);

    // remove all tracks in current prefix
    auto qobj = BSON("$dropall" << true);
    DEBUG_LOG(logject) << "query: " << qobj;
    auto qry = db.create_query(qobj, ec);
    if(ec) {
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
//    return;

    auto range = boost::make_iterator_range(fs::recursive_directory_iterator{dir}, {});

    for(fs::directory_entry& entry : range) {
        auto p = entry.path();
        if(!fs::is_regular_file(p))
            continue;
        TRACE_LOG(logject) << p;
        auto tracks = std::move(decman.openPath(p));
        if(tracks.empty())
            continue;
        coll.save_document(tracks.front().bson(), ec);
        if(ec) {
            ERROR_LOG(logject) << "document save error: " << ec.message();
            return;
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
//    auto qryall = db.create_query(BSON("path" << BSON("$begin" << dir.generic_string())), ec);
    auto all = coll.get_all();
    for(auto&& rec : all) {
        TRACE_LOG(logject) << rec;
    }

//    coll.set_index("path", 1 << 1);
//    coll.set_index("path", 1 << 7);
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
    pimpl->scanStarted();
//    unique_lock l(pimpl->mu);
//    l.unlock();
    pimpl->scanEnded();
}

Signals::Library::ScanStarted& Manager::getScanStartedSignal() noexcept { return pimpl->scanStarted; }

Signals::Library::ScanEnded& Manager::getScanEndedSignal() noexcept { return pimpl->scanEnded; }

} // namespace Library
} // namespace Melosic
