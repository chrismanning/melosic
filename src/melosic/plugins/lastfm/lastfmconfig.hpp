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

#ifndef LASTFMCONFIG_HPP
#define LASTFMCONFIG_HPP

#include <QPushButton>

#include <melosic/melin/config.hpp>
#include <melosic/gui/configwidget.hpp>

namespace Melosic {

class LastFmConfig : public Config::Config<LastFmConfig> {
public:
    LastFmConfig();

    virtual ~LastFmConfig();

    ConfigWidget* createWidget(Melosic::Config::Manager&) override;
    QIcon* getIcon() const override;
};

class LastFmConfigWidget : public GenericConfigWidget {
    Q_OBJECT
public:
    LastFmConfigWidget(Config::Base& conf, Melosic::Config::Manager& confman, QWidget* parent = nullptr);
    void apply();
private Q_SLOTS:
    void authenticate();
private:
    QPushButton* authButton;
};

} // end namespace Melosic

BOOST_CLASS_EXPORT_KEY(Melosic::LastFmConfig)

#endif // LASTFMCONFIG_HPP
