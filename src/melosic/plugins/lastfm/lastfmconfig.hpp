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

#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QPushButton>

#include <melosic/core/configuration.hpp>
#include <melosic/gui/configwidget.hpp>

namespace Melosic {

class LastFmConfig : public Configuration {
public:
    LastFmConfig();

    virtual ~LastFmConfig();

    ConfigWidget* createWidget() override;
    QIcon* getIcon() const override;
    LastFmConfig* clone() const override;

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & boost::serialization::base_object<Configuration>(*this);
    }
};

class LastFmConfigWidget : public ConfigWidget {
    Q_OBJECT
public:
    LastFmConfigWidget(Configuration::NodeMap& nodes, Configuration& conf);
private Q_SLOTS:
    void authenticate();
private:
    void apply();

    Configuration::NodeMap& nodes;
    Configuration& conf;
    QVBoxLayout* layout;
    QGroupBox* gen;
    QFormLayout* form;
    QPushButton* authButton;
};

} // end namespace Melosic

#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT_KEY(Melosic::LastFmConfig)

#endif // LASTFMCONFIG_HPP
