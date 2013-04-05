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

#ifndef CONFIGURATIONDIALOG_HPP
#define CONFIGURATIONDIALOG_HPP

#include <QDialog>
#include <QStackedLayout>
#include <QTreeWidget>
#include <QList>
#include <QDialogButtonBox>

#include <memory>

namespace Melosic {
namespace Config {
class Manager;
}

class ConfigWidget;

class ConfigurationDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ConfigurationDialog(Melosic::Config::Manager& confman, QWidget* parent = 0);
    ~ConfigurationDialog();

public Q_SLOTS:
    void apply();
    void defaults();

private Q_SLOTS:
    void changeConfigWidget(QTreeWidgetItem* item);

private:
    QStackedLayout* stackLayout;
    QTreeWidget* items;
    Melosic::Config::Manager& confman;
    QList<ConfigWidget*> visited;
    QDialogButtonBox* buttonBox;
};

} // namespace Melosic

#endif // CONFIGURATIONDIALOG_HPP
