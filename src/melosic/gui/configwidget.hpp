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

#ifndef MELOSIC_CONFIGWIDGET_HPP
#define MELOSIC_CONFIGWIDGET_HPP

#include <QWidget>

class QBoxLayout;
class QGroupBox;
class QFormLayout;
class QVariant;

#include <boost/variant/static_visitor.hpp>

namespace Melosic {
namespace Config {
class Manager;
class Base;
}

class ConfigWidget : public QWidget {
public:
    explicit ConfigWidget(Melosic::Config::Base&, QWidget* parent = nullptr);

    virtual void apply() = 0;
    virtual void restoreDefaults();
    virtual void setup() = 0;
protected:
    Melosic::Config::Base& conf;
};

class GenericConfigWidget : public ConfigWidget {
    Q_OBJECT
public:
    explicit GenericConfigWidget(Melosic::Config::Base&, QWidget* parent = nullptr);

    virtual void apply() override;
    virtual void setup() override;

protected:
    QBoxLayout* layout;
private:
    QGroupBox* gen;
    QFormLayout* form;
};

enum class ConfigType {
    String,
    Bool,
    Int,
    Float,
    Binary
};

std::vector<uint8_t> toVector(const QVariant&);
QVariant fromVector(const std::vector<uint8_t>&);

struct ConfigVisitor : boost::static_visitor<QWidget*> {
    QWidget* operator()(const std::string&);
    QWidget* operator()(bool);
    QWidget* operator()(int64_t);
    QWidget* operator()(double);
    QWidget* operator()(const std::vector<uint8_t>&);
};

} // namespace Melosic

#endif // MELOSIC_CONFIGWIDGET_HPP
