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

#ifndef MELOSIC_DECODERMANAGER_HPP
#define MELOSIC_DECODERMANAGER_HPP

#include <memory>
#include <functional>
#include <type_traits>

#include <boost/filesystem/path.hpp>
#include <boost/mpl/lambda.hpp>
namespace { namespace mpl = boost::mpl; }
#include <boost/optional/optional_fwd.hpp>

#include <taglib/tfile.h>

#include <melosic/melin/input.hpp>
#include <melosic/common/traits.hpp>

namespace Melosic {
namespace IO {
class File;
}

namespace Core {
class Track;
}

namespace Decoder {
typedef Input::Playable Playable;
typedef std::function<std::unique_ptr<Playable>(IO::BiDirectionalClosableSeekable&)> Factory;

class Manager {
public:
    Manager();
    ~Manager();

    Manager(Manager&&) = delete;
    Manager(const Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;

    MELOSIC_EXPORT void addAudioFormat(Factory fact, std::string extension);

    template <template <class...> class List, typename ...ListArgs>
    void addAudioFormat(Factory fact, List<std::string, ListArgs...> extensions) {
        for(std::string& ext : extensions) {
            addAudioFormat(fact, std::move(ext));
        }
    }

    template <typename ...Strings, class = typename std::enable_if<(sizeof...(Strings) > 1)>::type>
    void addAudioFormat(Factory fact, Strings&&... extensions) {
        static_assert(MultiArgsTrait<mpl::lambda<std::is_same<std::string, mpl::_1>>::type, Strings...>::value,
                      "extensions must be std::strings or convertible to");
        addAudioFormat(fact, {std::move(extensions)...});
    }

    boost::optional<Core::Track> openTrack(boost::filesystem::path filepath,
                            chrono::milliseconds start = 0ms,
                            chrono::milliseconds end = 0ms) const noexcept;

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Decoder
} // namespace Melosic

#endif // MELOSIC_DECODERMANAGER_HPP
