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

#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

#include <boost/algorithm/string.hpp>
namespace algo = boost::algorithm;

#include <melosic/melin/output.hpp>
#include <melosic/melin/sigslots/slots.hpp>
#include <melosic/melin/sigslots/signals.hpp>

#include "outputconfwidget.hpp"

namespace Melosic {
namespace Output {

ConfigWidget* Conf::createWidget(QWidget* parent) {
    return new ConfWidget(*this, parent);
}

ConfWidget::ConfWidget(Output::Conf& conf, QWidget* parent)
    : ConfigWidget(conf, parent)
{
    layout = new QVBoxLayout;
    setup();
    this->setLayout(layout);
}

ConfWidget::~ConfWidget() {
    cbx->deleteLater();
    form->deleteLater();
    gen->deleteLater();
    layout->deleteLater();
}

void ConfWidget::apply() {
    conf.putNode("output device", cbx->itemData(cbx->currentIndex()).toString().toStdString());
}

void ConfWidget::setup() {
    if(layout->count()) {
        layout->removeWidget(gen);
        gen->deleteLater();
        form->deleteLater();
        cbx->deleteLater();
    }
    gen = new QGroupBox("Output Settings");
    form = new QFormLayout;

    cbx = new QComboBox;
    cbx->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    auto ptr = static_cast<Melosic::Output::Conf*>(&conf);
    assert(ptr && ptr->outputFactories);
    std::string cur = ptr->existsNode("output device") ? boost::get<std::string>(ptr->getNode("output device")) : "";
    int idx = 0;
    for(const auto& dev : *ptr->outputFactories) {
        cbx->addItem(QString::fromStdString(algo::replace_all_copy(dev.first.getDesc(), "\n", ": ")),
                     QString::fromStdString(dev.first.getName()));
        if(dev.first.getName() == cur)
            cbx->setCurrentIndex(idx);
        else
            idx++;
    }

    form->addRow("Output Devices", cbx);
    gen->setLayout(form);

    layout->addWidget(gen);
}

} // namespace Output
} // namespace Melosic
