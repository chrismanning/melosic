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
#include <chrono>

#include <boost/filesystem.hpp>

namespace Melosic {

namespace Input {
class InputManager;
class Source;
}
namespace Output {
class OutputManager;
class DeviceSink;
}
namespace IO {
class BiDirectionalClosableSeekable;
}
class Track;
class OutputDeviceName;

class Kernel {
public:
    typedef std::function<std::unique_ptr<Input::Source>(IO::BiDirectionalClosableSeekable&)> InputFactory;
    typedef std::function<std::unique_ptr<Output::DeviceSink>(const OutputDeviceName&)> OutputFactory;

    ~Kernel();

    static Kernel& getInstance();

    void loadPlugin(const std::string& filepath);
    void loadAllPlugins();

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
    void addOutputDevices(OutputFactory fact, std::initializer_list<std::string> avail);
    void addOutputDevices(OutputFactory fact, const std::list<OutputDeviceName>& avail);

    std::unique_ptr<Input::Source> getDecoder(const boost::filesystem::path& file,
                                              IO::BiDirectionalClosableSeekable& input);
    std::unique_ptr<Output::DeviceSink> getOutputDevice(const std::string& devicename);
    std::list<OutputDeviceName> getOutputDeviceNames();

private:
    Kernel();
    Kernel(const Kernel&);
    void operator=(const Kernel&);

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

}

#endif // MELOSIC_CORE_KERNEL_HPP
