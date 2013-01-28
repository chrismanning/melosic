/**************************************************************************
**  Copyright (C) 2012 Christian Manning
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

#ifndef MELOSIC_CORE_KERNEL_HPP
#define MELOSIC_CORE_KERNEL_HPP

#include <memory>
#include <string>
#include <functional>

#include <boost/filesystem.hpp>
#include <boost/function.hpp>

#include <taglib/tfile.h>

#include <melosic/common/error.hpp>

namespace Melosic {

class Player;
class Configuration;

namespace Input {
class Source;
}
namespace Output {
class DeviceSink;
}
namespace IO {
class File;
class BiDirectionalClosableSeekable;
}
class OutputDeviceName;

class Kernel : public std::enable_shared_from_this<Kernel> {
public:
    typedef std::function<std::unique_ptr<Input::Source>(IO::BiDirectionalClosableSeekable&)> InputFactory;
    typedef std::function<std::unique_ptr<Output::DeviceSink>(const OutputDeviceName&)> OutputFactory;

    Kernel();

    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;

    ~Kernel();

    Player& getPlayer();

    void loadPlugin(const std::string& filepath);
    void loadAllPlugins();

    void addOutputDevices(OutputFactory fact, std::initializer_list<std::string> avail);
    void addOutputDevices(OutputFactory fact, const std::list<OutputDeviceName>& avail);

    Configuration& getConfig();
    void saveConfig();
    void loadConfig();

    std::unique_ptr<Output::DeviceSink> getOutputDevice(const std::string& devicename);
    std::list<OutputDeviceName> getOutputDeviceNames();

    void addInputExtension(InputFactory fact, const std::string& extension);
    template <typename List>
    void addInputExtensions(InputFactory fact, const List& extensions) {
        for(const auto& ext : extensions) {
            addInputExtension(fact, ext);
        }
    }
    template <typename String>
    void addInputExtensions(InputFactory fact, const std::initializer_list<String>& extensions) {
        for(const auto& ext : extensions) {
            addInputExtension(fact, ext);
        }
    }

    struct FileTypeResolver {
        FileTypeResolver(std::shared_ptr<Kernel> kernel, const boost::filesystem::path&);
        std::unique_ptr<Input::Source> getDecoder(IO::File& file);
        std::unique_ptr<TagLib::File> getTagReader(IO::File& file);

    private:
        InputFactory decoderFactory;
        std::unique_ptr<TagLib::IOStream> taglibFile;
        std::function<TagLib::File*(TagLib::IOStream*)> tagFactory;
    };

    FileTypeResolver getFileTypeResolver(const boost::filesystem::path& path) {
        return FileTypeResolver(shared_from_this(), path);
    }

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

class OutputDeviceName {
public:
    OutputDeviceName(std::string name) : OutputDeviceName(name, "") {}
    OutputDeviceName(std::string name, std::string desc) : name(name), desc(desc) {}
    const std::string& getName() const {
        return name;
    }
    const std::string& getDesc() const {
        return desc;
    }
    bool operator<(const OutputDeviceName& b) const {
        return name < b.name;
    }
    friend std::ostream& operator<<(std::ostream&, const OutputDeviceName&);

private:
    const std::string name, desc;
};
inline std::ostream& operator<<(std::ostream& out, const OutputDeviceName& b) {
    return out << b.name + (b.desc.size() ? (": " + b.desc) : "");
}

namespace ErrorTag {
typedef boost::error_info<struct tagDeviceName, std::string> DeviceName;
}

}

#endif // MELOSIC_CORE_KERNEL_HPP
