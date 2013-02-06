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

#include <QLabel>

#include <melosic/melin/config.hpp>
#include <melosic/common/string.hpp>
using namespace Melosic;

#include "configwidget.hpp"

ConfigWidget::ConfigWidget(Config::Base& conf, Config::Manager& confman, QWidget* parent) :
    QWidget(parent), conf(conf), confman(confman)
{}

void ConfigWidget::apply() {
    confman.saveConfig();
}

ConfigWidget* Config::Base::createWidget(Melosic::Config::Manager&) {
    return nullptr;
}

QIcon* Config::Base::getIcon() const {
    return nullptr;
}

GenericConfigWidget::GenericConfigWidget(Config::Base& conf, Config::Manager& confman, QWidget* parent)
    : ConfigWidget(conf, confman, parent)
{
    layout = new QVBoxLayout;
    gen = new QGroupBox("General");
    form = new QFormLayout;

    ConfigVisitor cv;
    for(const auto& node : conf.getNodes()) {
        auto label = new QLabel(QString(toTitle(node.first).c_str()));
        QWidget* w = node.second.apply_visitor(cv);
        if(!w)
            continue;
        w->setProperty("key", QString::fromStdString(node.first));
        form->addRow(label, w);
    }
    gen->setLayout(form);

    layout->addWidget(gen);
    this->setLayout(layout);
}

void GenericConfigWidget::apply() {
    for(int i=0; i<form->rowCount(); ++i) {
        auto item = form->itemAt(i, QFormLayout::FieldRole);
        if(QWidget* w = item->widget()) {
            Config::Base::VarType v;
            switch(ConfigType(w->property("type").toInt())) {
                case ConfigType::String:
                    if(auto le = dynamic_cast<QLineEdit*>(w)) {
                        v = le->text().toStdString();
                    }
                    break;
                case ConfigType::Bool:
                    if(auto cb = dynamic_cast<QCheckBox*>(w)) {
                        v = cb->isChecked();
                    }
                    break;
                case ConfigType::Int:
                    if(auto le = dynamic_cast<QLineEdit*>(w)) {
                        v = le->text().toLong();
                    }
                    break;
                case ConfigType::Float:
                    if(auto le = dynamic_cast<QLineEdit*>(w)) {
                        v = le->text().toDouble();
                    }
                    break;
                case ConfigType::Binary:
                    continue;
            }
            conf.putNode(w->property("key").toString().toStdString(), v);
        }
    }
    ConfigWidget::apply();
}

QWidget* ConfigVisitor::operator()(const std::string& val) {
    auto le = new QLineEdit(QString::fromStdString(val));
    le->setProperty("type", int(ConfigType::String));
    return le;
}

QWidget* ConfigVisitor::operator()(bool val) {
    auto cb = new QCheckBox;
    cb->setChecked(val);
    cb->setProperty("type", int(ConfigType::Bool));
    return cb;
}

QWidget* ConfigVisitor::operator()(int64_t val) {
    auto le = new QLineEdit(QString::number(val));
    le->setValidator(new QIntValidator);
    le->setProperty("type", int(ConfigType::Int));
    return le;
}

QWidget* ConfigVisitor::operator()(double val) {
    auto le = new QLineEdit(QString::number(val));
    le->setValidator(new QDoubleValidator);
    le->setProperty("type", int(ConfigType::Float));
    return le;
}

QWidget* ConfigVisitor::operator()(const std::vector<uint8_t>&) {
    return nullptr;
}
