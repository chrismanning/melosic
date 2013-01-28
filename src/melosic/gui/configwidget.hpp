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
#include <QLineEdit>
#include <QCheckBox>
#include <QDoubleValidator>
#include <QIntValidator>

#include <boost/variant/static_visitor.hpp>

class ConfigWidget : public QWidget {
    Q_OBJECT
public:
    explicit ConfigWidget(QWidget *parent = 0);

    virtual void apply() = 0;
};

enum class ConfigType {
    String,
    Bool,
    Int,
    Float
};

struct ConfigVisitor : boost::static_visitor<QWidget*> {
    QWidget* operator()(const std::string& val) {
        auto le = new QLineEdit(QString::fromStdString(val));
        le->setProperty("type", int(ConfigType::String));
        return le;
    }
    QWidget* operator()(bool val) {
        auto cb = new QCheckBox;
        cb->setChecked(val);
        cb->setProperty("type", int(ConfigType::Bool));
        return cb;
    }
    QWidget* operator()(int64_t val) {
        auto le = new QLineEdit(QString::number(val));
        le->setValidator(new QIntValidator);
        le->setProperty("type", int(ConfigType::Int));
        return le;
    }
    QWidget* operator()(double val) {
        auto le = new QLineEdit(QString::number(val));
        le->setValidator(new QDoubleValidator);
        le->setProperty("type", int(ConfigType::Float));
        return le;
    }
};

#endif // MELOSIC_CONFIGWIDGET_HPP
