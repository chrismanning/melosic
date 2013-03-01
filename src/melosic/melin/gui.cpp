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

#include <unordered_map>
#include <type_traits>

#include <QWidget>
#include <QString>

#include "gui.hpp"

namespace Melosic {
namespace GUI {

class Manager::impl {
public:
    impl() {}

    void addWidgetFactory(const std::string& type, WidgetFactory&& fact) {
        factories.emplace(type, std::forward<WidgetFactory>(fact));
    }

    QWidget* createWidget(const std::string& type, QWidget* parent) {
        auto i = factories.find(type);
        if(i == factories.end())
            return nullptr;
        return i->second(parent);
    }

private:
    std::unordered_map<std::string, WidgetFactory> factories;
};

Manager::Manager() : pimpl(new impl) {}

Manager::~Manager() {}

void Manager::addWidgetFactory(const std::string& type, WidgetFactory&& fact) {
    pimpl->addWidgetFactory(type, std::forward<WidgetFactory>(fact));
}

} // namespace GUI
} // namespace Melosic
