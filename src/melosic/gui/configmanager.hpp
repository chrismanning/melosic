/**************************************************************************
**  Copyright (C) 2014 Christian Manning
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

#ifndef MELOSIC_QML_CONFIGMANAGER_HPP
#define MELOSIC_QML_CONFIGMANAGER_HPP

#include <QObject>
#include <QAbstractItemModel>
#include <QWindow>
#include <QWidget>

#include <memory>

namespace Melosic {

namespace Config {
class Manager;
}

class ConfigManager : public QObject {
    Q_OBJECT
    ConfigManager();
    ~ConfigManager();

    struct impl;
    std::unique_ptr<impl> pimpl;

  public:
    static ConfigManager* instance();

    Config::Manager* getConfigManager() const;
    void setConfigManager(Config::Manager* confman);

    Q_INVOKABLE void openWindow();

    QWindow* getParent() const;
    void setParent(QWindow*);

Q_SIGNALS:
    void configManagerChanged();

  public Q_SLOTS:
};

class Configurator : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)

  public:
    using QObject::QObject;
    virtual ~Configurator() {}

  public Q_SLOTS:
    virtual bool modified() const = 0;
    virtual bool apply() = 0;
    virtual bool reset() = 0;
    virtual bool restore() = 0;

Q_SIGNALS:
    void modifiedChanged(bool modified);
};

class Configurable {
    Q_GADGET
  public:
    virtual Configurator* configure() const = 0;
};

} // namespace Melosic

#endif // MELOSIC_QML_CONFIGMANAGER_HPP
