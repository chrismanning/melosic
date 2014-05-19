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
#include <boost/container/flat_map.hpp>
#include <boost/scope_exit.hpp>

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
#include <melosic/common/thread.hpp>
#include "library.hpp"

namespace std {

template <typename Container, typename RepT, typename RatioT>
void value_set(jbson::basic_element<Container>& elem, std::chrono::duration<RepT, RatioT> dur) {
    elem.value(std::chrono::duration_cast<std::chrono::duration<RepT, std::milli>>(dur).count());
}

template <typename Container, typename ClockT, typename DurT>
void value_set(jbson::basic_element<Container>& elem, std::chrono::time_point<ClockT, DurT> time) {
    value_set(elem, time.time_since_epoch());
}

template <typename Container, typename RepT, typename RatioT>
void value_get(const jbson::basic_element<Container>& elem, std::chrono::duration<RepT, RatioT>& dur) {
    RepT count = get<RepT>(elem);
    dur = std::chrono::duration_cast<std::chrono::duration<RepT, RatioT>>(
                std::chrono::duration<RepT, std::milli>{count});
}

template <typename Container, typename ClockT, typename DurT>
void value_get(const jbson::basic_element<Container>& elem, std::chrono::time_point<ClockT, DurT>& time) {
    DurT dur;
    value_get(elem, dur);
    time = std::chrono::time_point<ClockT, DurT>{dur};
}

}

namespace Melosic {
namespace Library {

using std::mutex;
using unique_lock = std::unique_lock<mutex>;

struct ScanStarted : Signals::Signal<Signals::Library::ScanStarted> {};
struct ScanEnded : Signals::Signal<Signals::Library::ScanEnded> {};

struct Added : Signals::Signal<Signals::Library::Added> {};
struct Removed : Signals::Signal<Signals::Library::Removed> {};
struct Updated : Signals::Signal<Signals::Library::Updated> {};

static const fs::path DataDir{Directories::dataHome() / "melosic"};

struct PathEquivalence : std::binary_function<const fs::path&, const fs::path&, bool> {
    bool operator()(const fs::path& a, const fs::path& b) const { return fs::equivalent(a, b); }
};

struct Manager::impl {
    impl(Config::Manager&, Decoder::Manager&, Thread::Manager&);

    void loadedSlot(boost::synchronized_value<Config::Conf>& ubase);
    void variableUpdateSlot(const Config::KeyType& key, const Config::VarType& val);

    void scan();
    void scan(const fs::path& dir);

    void add(const fs::path& file);
    void add(const network::uri& uri);
    void remove(const fs::path& file);
    void remove(const network::uri& uri);
    void remove_prefix(const fs::path& dir);
    void update(const fs::path& file);
    void update(const network::uri& uri);
    std::vector<jbson::document> query(const jbson::document& qdoc);

    Decoder::Manager& decman;
    Thread::Manager& tman;
    Logger::Logger logject{logging::keywords::channel = "Library::Manager"};
    Config::Conf conf{"Library"};
    ejdb::db m_db;
    ScanStarted scanStarted;
    ScanEnded scanEnded;
    Added added;
    Removed removed;
    Updated updated;
    boost::synchronized_value<Manager::SetType> m_dirs;
    mutex mu;
    std::atomic<bool> pluginsLoaded{false};
    std::atomic<bool> m_scanning{false};
};

Manager::impl::impl(Config::Manager& confman, Decoder::Manager& decman, Thread::Manager& tman)
    : decman(decman), tman(tman) {
    if(!fs::exists(DataDir))
        fs::create_directory(DataDir);
    assert(fs::exists(DataDir) && fs::is_directory(DataDir));
    m_db.open((DataDir / "medialibrary").generic_string(),
            ejdb::db_mode::read | ejdb::db_mode::write | ejdb::db_mode::create);

    m_db.create_collection("tracks");

    confman.getLoadedSignal().connect(&impl::loadedSlot, this);

    added.connect([this](network::uri uri) {
        LOG(logject) << "Media added to library: " << uri;
    });
    removed.connect([this](network::uri uri) {
        LOG(logject) << "Media removed from library: " << uri;
    });
    updated.connect([this](network::uri uri) {
        LOG(logject) << "Media updated in library: " << uri;
    });

    scanStarted.connect([this]() {
        m_scanning.store(true);
    });
    scanEnded.connect([this]() {
        m_scanning.store(false);
        TRACE_LOG(logject) << "Setting indexes on \"tracks\" collection";
        std::error_code ec;
        auto coll = m_db.get_collection("tracks", ec);
        if(ec) {
            ERROR_LOG(logject) << "Could not get collection: " << ec.message();
            return;
        }
        assert(coll);
        coll.set_index("modified", ejdb::index_mode::number|ejdb::index_mode::rebuild|ejdb::index_mode::optimize, ec);
        if(ec) {
            ERROR_LOG(logject) << "Could not set index on collection field \"modified\": " << ec.message();
            ec.clear();
        }

        coll.set_index("location", ejdb::index_mode::string|ejdb::index_mode::rebuild|ejdb::index_mode::optimize, ec);
        if(ec) {
            ERROR_LOG(logject) << "Could not set index on collection field \"location\": " << ec.message();
            ec.clear();
        }

        coll.set_index("metadata", ejdb::index_mode::array|ejdb::index_mode::rebuild|ejdb::index_mode::optimize, ec);
        if(ec) {
            ERROR_LOG(logject) << "Could not set index on collection field \"metadata\": " << ec.message();
            ec.clear();
        }

        TRACE_LOG(logject) << "Indexes set on \"tracks\" collection";
    });
}

void Manager::impl::loadedSlot(boost::synchronized_value<Config::Conf>& ubase) {
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

    auto coll = m_db.get_collection("tracks");
    assert(coll);
    ejdb::unique_transaction trans(coll.transaction());

    c->iterateNodes([&](const std::tuple<Config::KeyType, Config::VarType>& pair) {
        using std::get;
        TRACE_LOG(logject) << "Config: variable loaded: " << get<0>(pair);
        variableUpdateSlot(get<0>(pair), get<1>(pair));
    });

    {
        TRACE_LOG(logject) << "Removing tracks not under a configured directory";

        auto dirs = m_dirs.synchronize();
        auto builder = jbson::builder("location", jbson::builder("$exists", true));
        for(auto&& dir : boost::make_iterator_range(dirs->begin(), dirs->end())) {
            TRACE_LOG(logject) << "Removing tracks not under prefix: " << dir;
            builder("location", jbson::builder("$not", jbson::builder("$begin", Input::to_uri(dir).string())));
        }
        builder("$dropall", true);
        auto qdoc = jbson::document(std::move(builder));
        auto qry = m_db.create_query(qdoc.data());

        auto n = coll.execute_query<ejdb::query_search_mode::count_only>(qry);
        LOG(logject) << n << " tracks removed";
    }

    c->getVariableUpdatedSignal().connect(&impl::variableUpdateSlot, this);
}

void Manager::impl::variableUpdateSlot(const Config::KeyType& key, const Config::VarType& val) {
    using std::get;
    TRACE_LOG(logject) << "Config: variable updated: " << key;
    try {
        if(key == "directories") {
            auto dirs = m_dirs.synchronize();
            auto config_vars = get<std::vector<Config::VarType>>(val);

            std::set<fs::path> config_dirs, old_dirs{dirs->begin(), dirs->end()};
            for(auto&& var : config_vars)
                config_dirs.insert(get<std::string>(var));

            std::vector<fs::path> missing_dirs;

            std::set_difference(old_dirs.begin(), old_dirs.end(), config_dirs.begin(), config_dirs.end(),
                                std::back_inserter(missing_dirs));
            for(auto&& p : missing_dirs)
                remove_prefix(p);

            missing_dirs.clear();
            for(auto&& p : config_dirs) {
                if(!fs::is_directory(p))
                    continue;
                auto ret = dirs->insert(fs::canonical(p));
                if(get<bool>(ret))
                    missing_dirs.push_back(p);
            }

            tman.enqueue([this, missing_dirs] {
                TRACE_LOG(logject) << "Scanning previously missing library dirs";
                scanStarted();
                BOOST_SCOPE_EXIT_ALL(this) { scanEnded(); };
                for(auto&& p : missing_dirs)
                    scan(p);
            });
        }
        else
            WARN_LOG(logject) << "Unknown variable: " << key;
    }
    catch(boost::bad_get&) {
        ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
        scanEnded();
    }
}

void Manager::impl::scan() {
    if(!pluginsLoaded) {
        WARN_LOG(logject) << "Plugins not yet loaded. Aborting scan.";
        return;
    }
    scanStarted();
    BOOST_SCOPE_EXIT_ALL(this) { scanEnded(); };
    m_dirs([this](auto&& dirs) {
        for(auto&& dir : dirs) {
            scan(dir);
        }
    });
}

void Manager::impl::scan(const fs::path& dir) {
    if(!pluginsLoaded) {
        WARN_LOG(logject) << "Plugins not yet loaded. Aborting scan.";
        return;
    }
    using jbson::get;

    auto coll = m_db.get_collection("tracks");
    assert(coll);
    if(!coll)
        return;

    if(!fs::exists(dir)) {
        ERROR_LOG(logject) << "Directory " << dir << " does not exist. Aborting scan.";
        return;
    }

    std::error_code ec;

    // query for finding all known files under this dir
    const auto qdoc = jbson::document(jbson::builder
                                      ("location", jbson::builder
                                       ("$begin", Input::to_uri(dir).string())
                                      )
                                     );

    try { // find and remove files that no longer exist
        TRACE_LOG(logject) << "Removing non-existent files";
        ejdb::unique_transaction trans(coll.transaction());
        assert(trans);

        auto under_dir = apply_named_paths(query(qdoc), {{"oid", "$._id"}, {"location", "$.location"}});

        for(auto&& entry : under_dir) {
            assert(entry.size() == 2);

            auto location = entry.begin();
            auto oid = entry.begin();
            if(location->name() != "location")
                location++;
            else if(oid->name() != "oid")
                oid++;
            assert(oid != entry.end());
            assert(location != entry.end());

            if(location->type() != jbson::element_type::string_element) {
                ERROR_LOG(logject) << "Location should be string_element; instead of " << location->type();
                continue;
            }

            network::uri uri;
            uri = network::make_uri(get<std::string>(*location), ec);
            if(ec) {
                ERROR_LOG(logject) << "Could not make uri: " << ec.message();
                continue;
            }

            if(uri.scheme() != boost::string_ref("file"))
                continue;

            boost::system::error_code bec;
            auto path = Input::uri_to_path(uri);

            if(fs::exists(path, bec) && !bec)
                continue;

            TRACE_LOG(logject) << "Removing file from library: " << path;
            coll.remove_document(get<jbson::element_type::oid_element>(*oid));
            removed(uri);
        }
    }
    catch(std::runtime_error& e) {
        ERROR_LOG(logject) << "Error removing files from db";
        ERROR_LOG(logject) << e.what();
        return;
    }
    catch(...) {
        ERROR_LOG(logject) << "Error removing files from db: " << boost::current_exception_diagnostic_information();
        return;
    }

    ec.clear();

    if(!fs::exists(dir) || !fs::is_directory(dir))
        return;

    try {
        TRACE_LOG(logject) << "Adding/updating files under " << dir;
        ejdb::unique_transaction trans(coll.transaction());
        auto under_dir = apply_named_paths(query(qdoc), {{"oid", "$._id"},
                                                         {"location", "$.location"},
                                                         {"modified", "$.modified"}});

        using lib_container = boost::container::flat_multimap<fs::path,
            std::tuple<std::chrono::system_clock::time_point, std::array<char, 12>>>;
        lib_container lib;

        for(auto&& set : under_dir) {
            assert(set.size() == 3);
            lib_container::value_type value;

            auto it = set.find("location");
            if(it == set.end() || it->type() != jbson::element_type::string_element) {
                ERROR_LOG(logject) << "Track has no location.";
                continue;
            }

            network::uri uri;
            uri = network::make_uri(get<std::string>(*it), ec);
            if(ec) {
                ERROR_LOG(logject) << "Track has invalid location uri: " << ec.message();
            }
            get<0>(value) = Input::uri_to_path(uri);

            it = set.find("modified");
            if(it == set.end() || it->type() != jbson::element_type::date_element) {
                ERROR_LOG(logject) << "Track has no modified time.";
                continue;
            }

            get<0>(get<1>(value)) = get<std::chrono::system_clock::time_point>(*it);

            it = set.find("oid");
            if(it == set.end() || it->type() != jbson::element_type::oid_element) {
                ERROR_LOG(logject) << "Track has no oid.";
                continue;
            }

            get<1>(get<1>(value)) = get<jbson::element_type::oid_element>(*it);

            lib.insert(std::move(value));
        }

        for(const fs::directory_entry& entry : fs::recursive_directory_iterator{dir}) {
            auto p = entry.path();
            if(!fs::is_regular_file(p))
                continue;
            auto tracks = lib.equal_range(p);
            try {
                if(boost::distance(tracks) == 0)
                    add(p);
                else {
                    auto modified = std::chrono::system_clock::from_time_t(fs::last_write_time(p));
                    bool needs_update = std::any_of(get<0>(tracks), get<1>(tracks), [modified] (auto&& track) {
                        return get<0>(get<1>(track)) < modified;
                    });
                    if(needs_update)
                        update(p);
                }
            }
            catch(std::exception& e) {
                // TODO: fix uri (source of this exception)
                ERROR_LOG(logject) << p << "; " << e.what();
                continue;
            }
        }
    }
    catch(std::runtime_error& e) {
        ERROR_LOG(logject) << "Error scanning files to db";
        ERROR_LOG(logject) << e.what();
        return;
    }
    catch(...) {
        ERROR_LOG(logject) << "Error scanning files to db: " << boost::current_exception_diagnostic_information();
        return;
    }
    TRACE_LOG(logject) << dir << " scanned";

#if 0&& !defined(NDEBUG)
    qry = db.create_query("{}"_json_doc.data(), ec);
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
        jbson::write_json(jbson::document(std::move(doc)), std::back_inserter(doc_str));
        DEBUG_LOG(logject) << "track: " << doc_str;
    }
#endif
}

void Manager::impl::add(const boost::filesystem::path& file) {
    assert(fs::exists(file) && fs::is_regular_file(file));

    add(Input::to_uri(file));
}

void Manager::impl::add(const network::uri& uri) {
    auto tracks = decman.tracks(uri);
    if(tracks.empty()) {
        ERROR_LOG(logject) << "Could not get tracks from media at " << uri;
        return;
    }
    for(const auto& track : tracks) {
        auto track_bson = track.bson();
        track_bson.emplace(track_bson.end(), "modified", jbson::element_type::date_element,
                           std::chrono::system_clock::now());

        auto coll = m_db.get_collection("tracks");
        assert(coll);
        auto oid = coll.save_document(track_bson.data());
        (void)oid;
//        assert(oid);
    }
    TRACE_LOG(logject) << uri << " contains " << tracks.size() << " tracks";
    added(uri);
}

void Manager::impl::remove(const boost::filesystem::path& file) {
    assert(!fs::exists(file) || fs::is_regular_file(file));

    remove(Input::to_uri(file));
}

void Manager::impl::remove(const network::uri& uri) {
    auto coll = m_db.get_collection("tracks");
    assert(coll);

    auto qdoc = jbson::document(jbson::builder
                                ("location", uri.string())
                                ("$dropall", true)
                               );
    auto qry = m_db.create_query(qdoc.data());

    auto n = coll.execute_query<ejdb::query_search_mode::count_only>(qry);
    LOG(logject) << n << " tracks removed";
    removed(uri);
}

void Manager::impl::remove_prefix(const boost::filesystem::path& dir) {
    auto uri = Input::to_uri(dir);
    auto coll = m_db.get_collection("tracks");
    assert(coll);

    auto qdoc = jbson::document(jbson::builder
                                ("location", jbson::builder
                                 ("$begin", uri.string())
                                )
                                ("$dropall", true)
                               );
    auto qry = m_db.create_query(qdoc.data());

    auto n = coll.execute_query<ejdb::query_search_mode::count_only>(qry);
    LOG(logject) << n << " tracks removed";
    removed(uri);
}

void Manager::impl::update(const boost::filesystem::path& file) {
    assert(fs::exists(file) && fs::is_regular_file(file));

    remove(Input::to_uri(file));
}

void Manager::impl::update(const network::uri& uri) {
    ERROR_LOG(logject) << "update library entry not implemented";
}

std::vector<jbson::document> Manager::impl::query(const jbson::document& qdoc) {
    auto q = m_db.create_query(qdoc.data()).set_hints(R"({ "$orderby": { "location": 1 } })"_json_doc.data());
    assert(q);
    auto coll = m_db.get_collection("tracks");
    assert(coll);

    std::vector<jbson::document> ret;

    for(auto&& data : coll.execute_query(q))
        ret.emplace_back(std::move(data));

    return ret;
}

Manager::Manager(Config::Manager& confman, Decoder::Manager& decman, Plugin::Manager& plugman, Thread::Manager& tman)
    : pimpl(new impl(confman, decman, tman)) {
    plugman.getPluginsLoadedSignal().connect([this](auto&&) {
        TRACE_LOG(pimpl->logject) << "Plugins loaded. Scanning all...";
        pimpl->pluginsLoaded.store(true);
        pimpl->tman.enqueue([this]() {
            pimpl->scan();
        });
    });
}

Manager::~Manager() {}

const boost::synchronized_value<Manager::SetType>& Manager::getDirectories() const { return pimpl->m_dirs; }

std::vector<jbson::document> Manager::query(const jbson::document& qdoc) const {
    try {
        auto coll = pimpl->m_db.get_collection("tracks");
        if(!coll)
            return {};
        ejdb::unique_transaction trans(coll.transaction());
        return pimpl->query(qdoc);
    }
    catch(std::runtime_error& e) {
        ERROR_LOG(pimpl->logject) << e.what();
        return {};
    }
    catch(...) {
        ERROR_LOG(pimpl->logject) << "Query error: " << boost::current_exception_diagnostic_information();
        return {};
    }
}

std::vector<jbson::document_set>
Manager::query(const jbson::document& q,
               ForwardRange<std::tuple<std::string, std::string>> paths) const {
    return apply_named_paths(query(q), paths);
}

std::vector<jbson::document_set>
Manager::query(const jbson::document& q,
               std::initializer_list<std::tuple<std::string, std::string>> paths) const {
    return apply_named_paths(query(q), paths);
}

bool Manager::scanning() const noexcept {
    return pimpl->m_scanning.load();
}

Signals::Library::ScanStarted& Manager::getScanStartedSignal() noexcept { return pimpl->scanStarted; }

Signals::Library::ScanEnded& Manager::getScanEndedSignal() noexcept { return pimpl->scanEnded; }

std::vector<jbson::element> apply_path(const std::vector<jbson::document>& docs, boost::string_ref path) {
    std::vector<jbson::element> vec;
    if(path.empty() || (path.size() == 1 && path.front() == '$')) {
        for(auto&& doc : docs)
            for(auto&& e : doc)
                vec.emplace_back(e);
        return std::move(vec);
    }
    for(auto&& doc : docs)
        for(auto&& e : jbson::path_select(doc, path))
            vec.emplace_back(e);
    return std::move(vec);
}

std::vector<jbson::document_set>
apply_named_paths(const std::vector<jbson::document>& docs, ForwardRange<std::tuple<std::string, std::string>> paths) {
    std::vector<jbson::document_set> res;
    for(auto&& doc : docs) {
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
apply_named_paths(const std::vector<jbson::document>& docs,
                  std::initializer_list<std::tuple<std::string, std::string>> paths) {
    const auto p = ForwardRange<std::tuple<std::string, std::string>>(paths);
    return apply_named_paths(std::move(docs), p);
}

} // namespace Library
} // namespace Melosic
