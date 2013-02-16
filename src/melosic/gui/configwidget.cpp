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

#include <melosic/common/stream.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/common/string.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/output.hpp>
using namespace Melosic;

#include "configwidget.hpp"

Logger::Logger logject(logging::keywords::channel = "ConfigWidget");

ConfigWidget::ConfigWidget(Config::Base& conf, QWidget* parent) :
    QWidget(parent), conf(conf)
{}

void ConfigWidget::restoreDefaults() {
    conf.resetToDefault();
    setup();
}

ConfigWidget* Config::Base::createWidget() {
    return new GenericConfigWidget(*this);
}

QIcon* Config::Base::getIcon() const {
    return nullptr;
}

GenericConfigWidget::GenericConfigWidget(Config::Base& conf, QWidget* parent)
    : ConfigWidget(conf, parent)
{
    layout = new QVBoxLayout;
    setup();
    this->setLayout(layout);
}

void GenericConfigWidget::apply() {
    for(int i=0; i<form->rowCount(); ++i) {
        auto item = form->itemAt(i, QFormLayout::FieldRole);
        if(QWidget* w = item->widget()) {
            Config::VarType v;
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
}

void GenericConfigWidget::setup() {
    ConfigVisitor cv;
    if(layout->count()) {
        layout->removeWidget(gen);
        gen->deleteLater();
        form->deleteLater();
    }
    gen = new QGroupBox("General");
    form = new QFormLayout;
    for(const auto& node : conf.getNodes()) {
        QWidget* w = node.second.apply_visitor(cv);
        if(!w)
            continue;
        w->setProperty("key", QString::fromStdString(node.first));
        form->addRow(QString::fromStdString(toTitle(node.first)), w);
    }
    gen->setLayout(form);
    layout->insertWidget(0, gen);
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
