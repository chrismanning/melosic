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

#include <network/uri.hpp>

#include <melosic/common/traits.hpp>
#include <melosic/common/audiospecs.hpp>

namespace Melosic {

namespace Input {
class Manager;
}
namespace Plugin {
class Manager;
}

namespace Core {
class Track;
struct AudioFile;
class Kernel;
}

struct PCMBuffer;

namespace Decoder {
class PCMSource;
typedef std::function<std::unique_ptr<PCMSource>(std::unique_ptr<std::istream>)> Factory;

class Manager final {
    explicit Manager(const std::shared_ptr<Input::Manager>&, const std::shared_ptr<Plugin::Manager>&);
    friend class Core::Kernel;
public:
    ~Manager();

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    MELOSIC_EXPORT void addAudioFormat(Factory fact, std::string_view mime_type);

    template <typename StringT, template <class...> class List, typename ...ListArgs>
    void addAudioFormat(Factory fact, List<StringT, ListArgs...> mime_types) {
        for(auto&& mime_type : mime_types)
            addAudioFormat(fact, std::move(mime_type));
    }

    template <typename ...Strings, class = typename std::enable_if<(sizeof...(Strings) > 1)>::type>
    void addAudioFormat(Factory fact, Strings&&... mime_types) {
        for(auto&& mime_type : {std::forward<Strings>(mime_types)...})
            addAudioFormat(fact, mime_type);
    }

    MELOSIC_EXPORT std::vector<Melosic::Core::Track> tracks(const network::uri&) const;
    MELOSIC_EXPORT std::vector<Melosic::Core::Track> tracks(const boost::filesystem::path&) const;

    std::unique_ptr<PCMSource> open(const Core::Track&) const;

private:
    struct impl;
    std::shared_ptr<impl> pimpl;
};

class PCMSource {
public:
    typedef char char_type;
    virtual ~PCMSource() {}
    virtual void seek(chrono::milliseconds dur) = 0;
    virtual chrono::milliseconds tell() const = 0;
    virtual chrono::milliseconds duration() const = 0;
    virtual AudioSpecs getAudioSpecs() const = 0;
    virtual size_t decode(PCMBuffer& buf, std::error_code& ec) = 0;
    virtual bool valid() const = 0;
    virtual void reset() = 0;
};

} // namespace Decoder
} // namespace Melosic

#endif // MELOSIC_DECODERMANAGER_HPP
