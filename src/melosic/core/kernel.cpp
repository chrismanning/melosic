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

#include <melosic/common/plugin.hpp>
#include <melosic/core/inputmanager.hpp>
#include <melosic/core/outputmanager.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/common/logging.hpp>

namespace Melosic {

class Kernel::impl {
public:
    impl(Kernel& k) : k(k), logject(boost::log::keywords::channel = "Kernel") {}
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
            pl->registerPluginObjects(k);
            loadedPlugins.insert(decltype(loadedPlugins)::value_type(filename, pl));
        }
        catch(PluginException& e) {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

    Input::InputManager& getInputManager() {
        return inman;
    }

    Output::OutputManager& getOutputManager() {
        return outman;
    }

private:
    std::map<std::string, std::shared_ptr<Plugin>> loadedPlugins;
    Input::InputManager inman;
    Output::OutputManager outman;
    Kernel& k;
    Logger::Logger logject;
};

Kernel::Kernel() : pimpl(new impl(*this)) {}

Kernel::~Kernel() {}

void Kernel::loadPlugin(const std::string& filepath) {
    pimpl->loadPlugin(filepath);
}

void Kernel::loadAllPlugins() {
//        foreach(pe; dirEntries("plugins", "*.so", SpanMode.depth)) {
//            if(pe.name.canFind("qt")) {
//                continue;
//            }
//            loadPlugin(pe.name);
//        }
}

Input::InputManager& Kernel::getInputManager() {
    return pimpl->getInputManager();
}

Output::OutputManager& Kernel::getOutputManager() {
    return pimpl->getOutputManager();
}

}

