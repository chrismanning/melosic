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

#include <boost/filesystem/path.hpp>
#include <boost/functional/hash_fwd.hpp>
#include <boost/thread/synchronized_value.hpp>

#include <melosic/common/optional_fwd.hpp>
#include <melosic/common/signal_fwd.hpp>

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

namespace Core {
struct AudioFile;
class Track;
}

namespace Signals {
namespace Library {
using ScanStarted = SignalCore<void()>;
using ScanEnded = SignalCore<void()>;
}
}

namespace Library {

struct PathEquivalence;

class Manager final {
    using SetType = std::unordered_set<boost::filesystem::path, boost::hash<boost::filesystem::path>, PathEquivalence>;
public:
    Manager(Config::Manager&, Decoder::Manager&, Plugin::Manager&);

    ~Manager();

    const boost::synchronized_value<SetType>& getDirectories() const;
    void scan();

    Signals::Library::ScanStarted& getScanStartedSignal() noexcept;
    Signals::Library::ScanEnded& getScanEndedSignal() noexcept;

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Library
} // namespace Melosic

#endif // MELOSIC_LIBRARY_HPP
