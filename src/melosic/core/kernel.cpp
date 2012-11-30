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
#include <melosic/core/player.hpp>

namespace Melosic {

static Logger::Logger logject(boost::log::keywords::channel = "Kernel");

class Kernel::impl {
public:
    impl() : player(new Player) {}

    void loadPlugin(const std::string& filepath) {
        path p(filepath);

        if(!exists(p) && !is_regular_file(p)) {
            BOOST_THROW_EXCEPTION(FileNotFoundException() << ErrorTag::FilePath(absolute(p)));
        }

        if(p.extension() != ".melin") {
            BOOST_THROW_EXCEPTION(PluginInvalidException() << ErrorTag::FilePath(absolute(p)));
        }

        auto filename = p.filename().string();

        if(loadedPlugins.find(filename) != loadedPlugins.end()) {
            ERROR_LOG(logject) << "Plugin already loaded: " << filepath << std::endl;
            return;
        }

        std::shared_ptr<Plugin::Plugin> pl(new Plugin::Plugin(p));
        loadedPlugins.insert({filename, pl});
    }

    template<class StringList>
    void addOutputDevices(Kernel::OutputFactory fact, StringList avail) {
        for(const auto& device : avail) {
            auto it = outputFactories.find(device);
            if(it == outputFactories.end()) {
                outputFactories.insert({device, fact});
            }
        }
    }

    std::unique_ptr<Output::DeviceSink> getOutputDevice(const std::string& devicename) {
        auto it = outputFactories.find(devicename);

        if(it == outputFactories.end()) {
            BOOST_THROW_EXCEPTION(DeviceNotFoundException() << ErrorTag::DeviceName(devicename));
        }

        return it->second(it->first);
    }

    std::list<OutputDeviceName> getOutputDeviceNames() {
        std::list<OutputDeviceName> names;
        for(const auto& name : outputFactories) {
            names.push_back(name.first);
        }
        return names;
    }

    std::shared_ptr<Player> getPlayer() {
        return player;
    }

private:
    std::map<std::string, std::shared_ptr<Plugin::Plugin>> loadedPlugins;
    std::map<OutputDeviceName, Kernel::OutputFactory> outputFactories;
    std::shared_ptr<Player> player;
};

std::unique_ptr<Kernel::FileTypeResolver::impl> Kernel::FileTypeResolver::pimpl;

static std::map<std::string, Kernel::InputFactory>& inputFactories() {
    static std::map<std::string, Kernel::InputFactory> instance;
    return instance;
}

class Kernel::FileTypeResolver::impl {
public:
    impl(const boost::filesystem::path& filepath) {
        auto ext = filepath.extension().string();

        auto fact = inputFactories().find(ext);

        if(fact != inputFactories().end()) {
            decoderFactory = fact->second;
        }
        else {
            decoderFactory = [](decltype(decoderFactory)::argument_type& a) -> decltype(decoderFactory)::result_type {
                try {
                    IO::File& b = dynamic_cast<IO::File&>(a);
                    BOOST_THROW_EXCEPTION(UnsupportedFileTypeException() <<
                                          ErrorTag::FilePath(absolute(b.filename())));
                }
                catch(std::bad_cast& e) {
                    BOOST_THROW_EXCEPTION(UnsupportedTypeException());
                }
            };
        }

        if(ext == ".flac") {
                tagFactory = std::bind([&](TagLib::IOStream* a, TagLib::ID3v2::FrameFactory* b) {
                                           return new TagLib::FLAC::File(a, b);
                                       },
                                       std::placeholders::_1,
                                       nullptr);
        }
        else {
            tagFactory = [&](TagLib::IOStream* /*a*/) -> TagLib::File* {
                BOOST_THROW_EXCEPTION(MetadataUnsupportedException() <<
                                      ErrorTag::FilePath(absolute(filepath)));
            };
        }
    }

    std::unique_ptr<Input::Source> getDecoder(IO::File& file) {
        return decoderFactory(file);
    }

    std::unique_ptr<TagLib::File> getTagReader(IO::File& file) {
        taglibFile.reset(new IO::TagLibFile(file));
        return std::unique_ptr<TagLib::File>(tagFactory(taglibFile.get()));
    }

    static void addInputExtension(Kernel::InputFactory fact, const std::string& extension) {
        auto bef = inputFactories().size();
        auto pos = inputFactories().find(extension);
        if(pos == inputFactories().end()) {
            inputFactories().insert(std::remove_reference<decltype(inputFactories())>::type::value_type(extension, fact));
        }
        else {
            WARN_LOG(logject) << extension << ": already registered to a decoder factory";
        }
        assert(++bef == inputFactories().size());
    }

private:
    InputFactory decoderFactory;
    std::unique_ptr<TagLib::IOStream> taglibFile;
    std::function<TagLib::File*(TagLib::IOStream*)> tagFactory;
};

Kernel::FileTypeResolver::FileTypeResolver(const boost::filesystem::path& filepath) {
    pimpl.reset(new impl(filepath));
}

void Kernel::FileTypeResolver::addInputExtension(Kernel::InputFactory fact, const std::string& extension) {
    pimpl->addInputExtension(fact, extension);
}

std::unique_ptr<Input::Source> Kernel::FileTypeResolver::getDecoder(IO::File& file) {
    return std::move(pimpl->getDecoder(file));
}

std::unique_ptr<TagLib::File> Kernel::FileTypeResolver::getTagReader(IO::File& file) {
    return std::move(pimpl->getTagReader(file));
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

std::shared_ptr<Player> Kernel::getPlayer() {
    return pimpl->getPlayer();
}

} // end namespace Melosic

