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

#include <map>
#include <boost/filesystem.hpp>
using boost::filesystem::path;

#include <taglib/flacfile.h>

#include <melosic/common/file.hpp>
#include <melosic/common/plugin.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/common/logging.hpp>
#include <melosic/core/track.hpp>
#include <melosic/managers/output/pluginterface.hpp>
#include <melosic/core/taglibfile.hpp>

namespace Melosic {

static Logger::Logger logject(boost::log::keywords::channel = "Kernel");

class Kernel::impl {
public:
    impl() {}

    void loadPlugin(const std::string& filepath) {
        path p(filepath);

        enforceEx<Exception>(exists(p), (filepath + ": file does not exist").c_str());

        enforceEx<Exception>(p.extension() == ".melin" && is_regular_file(p),
                             (filepath + ": not a melosic plugin").c_str());

        try {
            auto filename = p.filename().string();

            if(loadedPlugins.find(filename) != loadedPlugins.end()) {
                ERROR_LOG(logject) << "Plugin already loaded: " << filepath << std::endl;
                return;
            }

            std::shared_ptr<Plugin> pl(new Plugin(p));
            pl->registerPluginObjects();
            loadedPlugins.insert(decltype(loadedPlugins)::value_type(filename, pl));
        }
        catch(PluginException& e) {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

    template<class StringList>
    void addOutputDevices(Kernel::OutputFactory fact, StringList avail) {
        for(auto device : avail) {
            auto it = outputFactories.find(device);
            if(it == outputFactories.end()) {
                outputFactories.insert(typename decltype(outputFactories)::value_type(device, fact));
            }
        }
    }

    std::unique_ptr<Output::DeviceSink> getOutputDevice(const std::string& devicename) {
        auto it = outputFactories.find(devicename);

        if(it == outputFactories.end()) {
            throw Exception("No such output device");
        }

        return std::unique_ptr<Output::DeviceSink>(it->second(it->first));
    }

    std::list<OutputDeviceName> getOutputDeviceNames() {
        std::list<OutputDeviceName> names;
        for(const auto& name : outputFactories) {
            names.push_back(name.first);
        }
        return names;
    }

private:
    std::map<std::string, std::shared_ptr<Plugin>> loadedPlugins;
    std::map<OutputDeviceName, Kernel::OutputFactory> outputFactories;
};

std::unique_ptr<Kernel::FileTypeResolver::impl> Kernel::FileTypeResolver::pimpl;


static std::map<std::string, Kernel::InputFactory>& inputFactories() {
    static std::map<std::string, Kernel::InputFactory> instance;
    return instance;
}

class Kernel::FileTypeResolver::impl {
public:
    impl(IO::File& file) : file(file) {}

    std::unique_ptr<Input::Source> getDecoder() {
        auto ext = file.filename().extension().string();

        auto fact = inputFactories().find(ext);

        enforceEx<Exception>(fact != inputFactories().end(), (file.filename().string() + ": cannot decode file").c_str());
        return fact->second(file);
    }

    std::unique_ptr<TagLib::File> getTagReader() {
        auto ext = file.filename().extension();
        taglibfile.reset(new IO::TagLibFile(file));
        if(ext == ".flac") {
            return std::unique_ptr<TagLib::File>(new TagLib::FLAC::File(taglibfile.get(), nullptr));
        }
        return nullptr;
    }

    static void addInputExtension(Kernel::InputFactory fact, const std::string& extension) {
        auto bef = inputFactories().size();
        auto pos = inputFactories().find(extension);
        if(pos == inputFactories().end()) {
            inputFactories().insert(std::remove_reference<decltype(inputFactories())>::type::value_type(extension, fact));
        }
        else {
            WARN_LOG(logject) << extension << ": already registered to decoder factory";
        }
        assert(++bef == inputFactories().size());
    }

private:
    IO::File& file;
    std::unique_ptr<IO::TagLibFile> taglibfile;
};

Kernel::FileTypeResolver::FileTypeResolver(IO::File& file) {
    pimpl.reset(new impl(file));
}

void Kernel::FileTypeResolver::addInputExtension(Kernel::InputFactory fact, const std::string& extension) {
    pimpl->addInputExtension(fact, extension);
}

std::unique_ptr<Input::Source> Kernel::FileTypeResolver::getDecoder() {
    return std::move(pimpl->getDecoder());
}

std::unique_ptr<TagLib::File> Kernel::FileTypeResolver::getTagReader() {
    return std::move(pimpl->getTagReader());
}

Kernel::Kernel() : pimpl(new impl) {}

Kernel::~Kernel() {}

Kernel& Kernel::getInstance() {
    static Kernel instance;
    return instance;
}

void Kernel::loadPlugin(const std::string& filepath) {
    pimpl->loadPlugin(filepath);
}

void Kernel::loadAllPlugins() {}

void Kernel::addOutputDevices(OutputFactory fact, std::initializer_list<std::string> avail) {
    pimpl->addOutputDevices(fact, avail);
}

void Kernel::addOutputDevices(OutputFactory fact, const std::list<OutputDeviceName>& avail) {
    pimpl->addOutputDevices(fact, avail);
}

std::unique_ptr<Output::DeviceSink> Kernel::getOutputDevice(const std::string& devicename) {
    return pimpl->getOutputDevice(devicename);
}

std::list<OutputDeviceName> Kernel::getOutputDeviceNames() {
    return pimpl->getOutputDeviceNames();
}

} // end namespace Melosic

