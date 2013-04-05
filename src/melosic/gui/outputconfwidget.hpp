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

#ifndef OUTPUTCONFWIDGET_HPP
#define OUTPUTCONFWIDGET_HPP

#include <QComboBox>

#include <boost/serialization/export.hpp>

#include <melosic/melin/sigslots/signals_fwd.hpp>

#include "configwidget.hpp"

namespace Melosic {
namespace Slots {
class Manager;
}
namespace Output {
class Conf;

class ConfWidget : public ConfigWidget {
public:
    ConfWidget(Melosic::Output::Conf&, QWidget* parent = nullptr);

    virtual ~ConfWidget();

    void apply() override;
    void setup() override;

private:
    QBoxLayout* layout;
    QGroupBox* gen;
    QFormLayout* form;
    QComboBox* cbx;
};

} // namespace Output
} // namespace Melosic

#endif // OUTPUTCONFWIDGET_HPP
