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

#include <melosic/common/file.hpp>
#include <melosic/common/plugin.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/common/logging.hpp>
#include <melosic/core/track.hpp>
#include <melosic/managers/output/pluginterface.hpp>

namespace Melosic {

class Kernel::impl {
public:
    impl() : logject(boost::log::keywords::channel = "Kernel") {}

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

    void addInputExtension(Kernel::InputFactory fact, const std::string& extension) {
        auto bef = inputFactories.size();
        auto pos = inputFactories.find(extension);
        if(pos == inputFactories.end()) {
            inputFactories.insert(decltype(inputFactories)::value_type(extension, fact));
        }
        else {
            WARN_LOG(logject) << extension << ": already registered to decoder factory";
        }
        assert(++bef == inputFactories.size());
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

    Kernel::InputFactory getInputFactory(const std::string& filename) {
        auto ext = path(filename).extension().string();

        auto fact = inputFactories.find(ext);

        enforceEx<Exception>(fact != inputFactories.end(), (filename + ": cannot decode file").c_str());
        return fact->second;
    }

    std::unique_ptr<Input::Source> getDecoder(const boost::filesystem::path& file,
                                              IO::BiDirectionalClosableSeekable& input) {
        return getInputFactory(file.string())(input);
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
    Logger::Logger logject;
    std::map<std::string, Kernel::InputFactory> inputFactories;
    std::map<OutputDeviceName, Kernel::OutputFactory> outputFactories;
};

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

void Kernel::addInputExtension(InputFactory fact, const std::string& extension) {
    pimpl->addInputExtension(fact, extension);
}

void Kernel::addOutputDevices(OutputFactory fact, std::initializer_list<std::string> avail) {
    pimpl->addOutputDevices(fact, avail);
}

void Kernel::addOutputDevices(OutputFactory fact, const std::list<OutputDeviceName>& avail) {
    pimpl->addOutputDevices(fact, avail);
}

std::unique_ptr<Input::Source> Kernel::getDecoder(const boost::filesystem::path& file,
                                                  IO::BiDirectionalClosableSeekable& input) {
    return pimpl->getDecoder(file, input);
}

std::unique_ptr<Output::DeviceSink> Kernel::getOutputDevice(const std::string& devicename) {
    return pimpl->getOutputDevice(devicename);
}

std::list<OutputDeviceName> Kernel::getOutputDeviceNames() {
    return pimpl->getOutputDeviceNames();
}

} // end namespace Melosic

