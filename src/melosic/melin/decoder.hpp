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

#include <openssl/md5.h>

#include <boost/filesystem/path.hpp>
#include <boost/mpl/lambda.hpp>
namespace {
namespace mpl = boost::mpl;
}

#include <cpprest/uri.h>

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
struct provider;

class Manager final {
    explicit Manager(const std::shared_ptr<Input::Manager>&, const std::shared_ptr<Plugin::Manager>&);
    friend class Core::Kernel;

  public:
    ~Manager();

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    void add_provider(std::shared_ptr<provider>);

    MELOSIC_EXPORT std::vector<Melosic::Core::Track> tracks(const web::uri&) const;
    MELOSIC_EXPORT std::vector<Melosic::Core::Track> tracks(const boost::filesystem::path&) const;

    std::unique_ptr<PCMSource> open(const Core::Track&) const;

  private:
    struct impl;
    std::shared_ptr<impl> pimpl;
};

MELOSIC_EXPORT std::array<unsigned char, MD5_DIGEST_LENGTH>
get_pcm_md5(std::unique_ptr<Melosic::Decoder::PCMSource> source);

struct provider {
    virtual ~provider() {
    }

    virtual bool supports_mime(std::string_view mime_type) const = 0;
    virtual std::unique_ptr<PCMSource> make_decoder(std::unique_ptr<std::istream> in) const = 0;
    virtual bool verify(std::unique_ptr<std::istream> in) const = 0;
};

class PCMSource {
  public:
    typedef char char_type;
    virtual ~PCMSource() {
    }
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
