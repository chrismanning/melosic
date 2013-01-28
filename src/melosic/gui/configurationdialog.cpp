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

#include "configurationdialog.hpp"
#include "configwidget.hpp"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QPushButton>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/indirected.hpp>
using namespace boost::adaptors;

#include <melosic/core/kernel.hpp>
#include <melosic/core/configuration.hpp>
#include <melosic/core/logging.hpp>
using namespace Melosic;

static Logger::Logger logject(boost::log::keywords::channel = "ConfigurationDialog");

ConfigurationDialog::ConfigurationDialog(std::shared_ptr<Kernel> kernel, QWidget* parent) :
    QDialog(parent),
    stackLayout(new QStackedLayout),
    items(new QTreeWidget),
    kernel(kernel)
{
    this->setWindowTitle("Configure");
    int i = 0;
    items->header()->hide();
    auto& c = kernel->getConfig();
//    std::clog << "In ConfDialog\n" << c << std::endl;
    for(Configuration& conf : c.getChildren() | map_values | indirected) {
        QString str = QString::fromStdString(conf.getName());
        auto w = conf.createWidget();
        if(str.size() && w) {
            QStringList strs(str);
            QTreeWidgetItem* item = new QTreeWidgetItem(strs);
            item->setData(0, Qt::UserRole, i);
            items->insertTopLevelItem(i, item);
            stackLayout->addWidget(w);
            ++i;
        }
    }
    stackLayout->setMargin(0);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Apply |
                                                       QDialogButtonBox::Cancel |
//                                                       QDialogButtonBox::RestoreDefaults |
                                                       QDialogButtonBox::Reset);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &ConfigurationDialog::apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigurationDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfigurationDialog::reject);

    connect(items, &QTreeWidget::currentItemChanged, this, &ConfigurationDialog::changeConfigWidget);

    QGridLayout* gridlayout = new QGridLayout;
    gridlayout->addWidget(items, 0, 0, 2, 1);
    gridlayout->addLayout(stackLayout, 0, 1, 1, 1);
    gridlayout->addWidget(buttonBox, 1, 1, 1, 2);
    gridlayout->setColumnStretch(1, 4);

    this->setLayout(gridlayout);
}

ConfigurationDialog::~ConfigurationDialog() {}

void ConfigurationDialog::changeConfigWidget(QTreeWidgetItem* item) {
    stackLayout->setCurrentIndex(item->data(0, Qt::UserRole).toInt());
    if(auto w = static_cast<ConfigWidget*>(stackLayout->currentWidget())) {
        visited.push_back(w);
    }
}

void ConfigurationDialog::apply() {
    TRACE_LOG(logject) << "Applying config";
    for(auto& w : visited) {
        w->apply();
    }
//    Kernel::getInstance().saveConfig();
}
