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

#ifndef MELOSIC_LIBRARY_HPP
#define MELOSIC_LIBRARY_HPP

#include <memory>
#include <unordered_set>
#include <unordered_map>

#include <boost/filesystem/path.hpp>
#include <boost/functional/hash_fwd.hpp>
#include <boost/thread/synchronized_value.hpp>
#include <boost/utility/string_ref_fwd.hpp>

#include <ejpp/ejdb.hpp>

#include <jbson/element_fwd.hpp>
#include <jbson/document_fwd.hpp>

#include <network/uri.hpp>

#include <melosic/common/optional_fwd.hpp>
#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/range.hpp>

namespace Melosic {

namespace Config {
class Manager;
}
namespace Decoder {
class Manager;
}
namespace Plugin {
class Manager;
}
namespace Thread {
class Manager;
}

namespace Core {
class Track;
}

namespace Signals {
namespace Library {
using ScanStarted = SignalCore<void()>;
using ScanEnded = SignalCore<void()>;

using Added = SignalCore<void(network::uri)>;
using Removed = SignalCore<void(network::uri)>;
using Updated = SignalCore<void(network::uri)>;
}
}

namespace Library {

struct PathEquivalence;

class Manager final {
    using SetType = std::unordered_set<boost::filesystem::path, boost::hash<boost::filesystem::path>, PathEquivalence>;
public:
    Manager(Config::Manager&, Decoder::Manager&, Plugin::Manager&, Thread::Manager&);

    ~Manager();

    MELOSIC_EXPORT const boost::synchronized_value<SetType>& getDirectories() const;

    MELOSIC_EXPORT
    std::vector<jbson::document> query(const jbson::document&) const;

    MELOSIC_EXPORT std::vector<jbson::document_set>
    query(const jbson::document&, ForwardRange<std::tuple<std::string, std::string> >) const;

    MELOSIC_EXPORT std::vector<jbson::document_set>
    query(const jbson::document&, std::initializer_list<std::tuple<std::string, std::string>>) const;

    MELOSIC_EXPORT Signals::Library::ScanStarted& getScanStartedSignal() noexcept;
    MELOSIC_EXPORT Signals::Library::ScanEnded& getScanEndedSignal() noexcept;

    MELOSIC_EXPORT bool scanning() const noexcept;

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

MELOSIC_EXPORT
std::vector<jbson::element> apply_path(const std::vector<jbson::document>&, boost::string_ref);

MELOSIC_EXPORT
std::vector<jbson::document_set> apply_named_paths(const std::vector<jbson::document>&,
                                                  ForwardRange<std::tuple<std::string, std::string>>);
MELOSIC_EXPORT
std::vector<jbson::document_set> apply_named_paths(const std::vector<jbson::document>&,
                                                  std::initializer_list<std::tuple<std::string, std::string>>);

} // namespace Library
} // namespace Melosic

#endif // MELOSIC_LIBRARY_HPP
