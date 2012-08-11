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

#include <melosic/common/ikernel.hpp>
#include <melosic/core/inputmanager.hpp>
#include <melosic/core/outputmanager.hpp>

namespace Melosic {

class Kernel : public IKernel {
public:
    void loadPlugin(const std::string& filepath) {
        path p(filepath);

        enforceEx<Exception>(exists(p),
                                    [&filepath]() {
                                        return (filepath + ": file does not exist").c_str();
                                    });

        enforceEx<Exception>(p.extension() == ".melin" && is_regular_file(p),
                                    [&filepath]() {
                                        return (filepath + ": not a melosic plugin").c_str();
                                    });

        try {
            auto const& filename = p.filename().string();

            if(loadedPlugins.find(filename) != loadedPlugins.end()) {
                std::cerr << "Plugin already loaded: " << filepath << std::endl;
                return;
            }

            std::shared_ptr<Plugin> pl(new Plugin(p));
            pl->registerPluginObjects(*this);
            loadedPlugins.insert(decltype(loadedPlugins)::value_type(filename, pl));
        }
        catch(PluginException& e) {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

   void loadAllPlugins() {
//        foreach(pe; dirEntries("plugins", "*.so", SpanMode.depth)) {
//            if(pe.name.canFind("qt")) {
//                continue;
//            }
//            loadPlugin(pe.name);
//        }
    }

    Input::IInputManager& getInputManager() {
        return inman;
    }

    Output::IOutputManager& getOutputManager() {
        return outman;
    }

private:
    std::map<std::string, std::shared_ptr<Plugin>> loadedPlugins;
    Input::InputManager inman;
    Output::OutputManager outman;
};

}

#endif // MELOSIC_CORE_KERNEL_HPP
