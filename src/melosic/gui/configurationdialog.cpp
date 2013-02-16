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
#include <QPushButton>

#include <boost/range/adaptor/indirected.hpp>
using namespace boost::adaptors;

#include <melosic/core/kernel.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/melin/logging.hpp>
using namespace Melosic;

static Logger::Logger logject(logging::keywords::channel = "ConfigurationDialog");

void addChildren(QStackedLayout* stack, QTreeWidgetItem* item, ForwardRange<Config::Base* const> b) {
    if(b.empty()) {
        return;
    }
    for(Config::Base& conf : b | indirected) {
        QString str = QString::fromStdString(conf.getName());
        auto w = conf.createWidget();
        if(str.size() && w) {
            TRACE_LOG(logject) << "Adding config page: " << conf.getName();
            QStringList strs(str);
            QTreeWidgetItem* ci = new QTreeWidgetItem(strs);
            const int t = stack->addWidget(w);
            TRACE_LOG(logject) << "Widget index: " << t;
            item->addChild(ci);
            ci->setData(0, Qt::UserRole, t);
            addChildren(stack, ci, conf.getChildren());
        }
    }
}

ConfigurationDialog::ConfigurationDialog(Config::Manager& confman, QWidget* parent) :
    QDialog(parent),
    stackLayout(new QStackedLayout),
    items(new QTreeWidget),
    confman(confman)
{
    this->setWindowTitle("Configure");
    items->header()->hide();
    auto& c = confman.getConfigRoot();
    for(Config::Base& conf : c.getChildren() | indirected) {
        QString str = QString::fromStdString(conf.getName());
        auto w = conf.createWidget();
        if(str.size() && w) {
            TRACE_LOG(logject) << "Adding config page: " << conf.getName();
            QStringList strs(str);
            QTreeWidgetItem* item = new QTreeWidgetItem(strs);
            const int t = stackLayout->addWidget(w);
            TRACE_LOG(logject) << "Widget index: " << t;
            item->setData(0, Qt::UserRole, t);
            items->addTopLevelItem(item);
            addChildren(stackLayout, item, conf.getChildren());
        }
    }
    stackLayout->setMargin(0);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                     QDialogButtonBox::Apply |
                                     QDialogButtonBox::Cancel |
                                     QDialogButtonBox::RestoreDefaults |
                                     QDialogButtonBox::Reset);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &ConfigurationDialog::apply);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &ConfigurationDialog::defaults);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigurationDialog::apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigurationDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfigurationDialog::reject);

    connect(items, &QTreeWidget::currentItemChanged, this, &ConfigurationDialog::changeConfigWidget);

    QGridLayout* gridlayout = new QGridLayout;
    gridlayout->addWidget(items, 0, 0, 2, 1);
    gridlayout->addLayout(stackLayout, 0, 1, 1, 1);
    gridlayout->addWidget(buttonBox, 1, 1, 1, 2);
    gridlayout->setColumnStretch(0, 1);
    gridlayout->setColumnStretch(1, 3);

    this->setLayout(gridlayout);
}

ConfigurationDialog::~ConfigurationDialog() {}

void ConfigurationDialog::changeConfigWidget(QTreeWidgetItem* item) {
    stackLayout->setCurrentIndex(item->data(0, Qt::UserRole).toInt());
    if(auto w = static_cast<ConfigWidget*>(stackLayout->currentWidget())) {
        visited.push_back(w);
        connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, w, &ConfigWidget::setup);
    }
}

void ConfigurationDialog::apply() {
    TRACE_LOG(logject) << "Applying config";
    for(auto& w : visited) {
        w->apply();
    }
    confman.saveConfig();
}

void ConfigurationDialog::defaults() {
    TRACE_LOG(logject) << "Restoring default config";
    assert(!visited.empty());
    visited.back()->restoreDefaults();
}
