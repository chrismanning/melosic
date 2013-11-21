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
#include <future>

#include <boost/filesystem/path.hpp>
#include <boost/mpl/lambda.hpp>
namespace { namespace mpl = boost::mpl; }

#include <melosic/common/optional_fwd.hpp>
#include <melosic/common/traits.hpp>
#include <melosic/common/audiospecs.hpp>

namespace Melosic {
namespace Thread {
class Manager;
}

namespace Core {
class Track;
struct AudioFile;
}

struct PCMBuffer;

namespace Decoder {
class PCMSource;
typedef std::function<std::unique_ptr<PCMSource>(std::unique_ptr<std::istream>)> Factory;

class Manager final {
public:
    explicit Manager(Thread::Manager&);
    ~Manager();

    Manager(Manager&&) = delete;
    Manager(const Manager&) = delete;
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

    optional<Core::AudioFile> getFile(boost::filesystem::path) const;
    std::future<bool> initialiseAudioFile(Core::AudioFile&) const;
    std::vector<Melosic::Core::Track> openPath(boost::filesystem::path) const;

    std::unique_ptr<PCMSource> openTrack(const Core::Track&) const;

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

class PCMSource {
public:
    typedef char char_type;
    virtual ~PCMSource() {}
    virtual void seek(chrono::milliseconds dur) = 0;
    virtual chrono::milliseconds tell() = 0;
    virtual chrono::milliseconds duration() const = 0;
    virtual AudioSpecs getAudioSpecs() = 0;
    virtual size_t decode(PCMBuffer& buf, boost::system::error_code& ec) = 0;
    virtual bool valid() = 0;
    virtual void reset() = 0;
};

} // namespace Decoder
} // namespace Melosic

#endif // MELOSIC_DECODERMANAGER_HPP
