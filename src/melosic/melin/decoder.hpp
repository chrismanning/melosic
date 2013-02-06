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

#include <boost/filesystem/path.hpp>

#include <taglib/tfile.h>

#include <melosic/melin/input.hpp>

namespace Melosic {
namespace Decoder {
typedef Input::Playable Playable;
}
}
extern template class std::function<std::unique_ptr<Melosic::Decoder::Playable>(Melosic::IO::BiDirectionalClosableSeekable&)>;
namespace Melosic {
namespace Decoder {
typedef std::function<std::unique_ptr<Playable>(IO::BiDirectionalClosableSeekable&)> Factory;
}
namespace IO {
class File;
}
namespace Decoder {
class Manager;
struct FileTypeResolver {
    FileTypeResolver(Manager& decman, const boost::filesystem::path&);
    std::unique_ptr<Decoder::Playable> getDecoder(IO::File& file);
    std::unique_ptr<TagLib::File> getTagReader(IO::File& file);

private:
    Factory decoderFactory;
    std::unique_ptr<TagLib::IOStream> taglibFile;
    std::function<TagLib::File*(TagLib::IOStream*)> tagFactory;
};

class Manager {
public:
    Manager();
    ~Manager();

    Manager(Manager&&) = delete;
    Manager(const Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;

    void addAudioFormat(Factory fact, const std::string& extension);
    template <typename List>
    void addAudioFormats(Factory fact, const List& extensions) {
        for(const auto& ext : extensions) {
            addAudioFormat(fact, ext);
        }
    }
    template <typename String>
    void addAudioFormats(Factory fact, const std::initializer_list<String>& extensions) {
        for(const auto& ext : extensions) {
            addAudioFormat(fact, ext);
        }
    }

    FileTypeResolver getFileTypeResolver(const boost::filesystem::path& path) {
        return FileTypeResolver(*this, path);
    }

private:
    class impl;
    std::unique_ptr<impl> pimpl;
    friend struct FileTypeResolver;
};

} // namespace Decoder
} // namespace Melosic

#endif // MELOSIC_DECODERMANAGER_HPP
