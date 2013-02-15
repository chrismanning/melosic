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

#ifndef MELOSIC_CONFIGURATION_HPP
#define MELOSIC_CONFIGURATION_HPP

class QIcon;

#include <memory>
#include <vector>
#include <type_traits>
#include <array>

#include <boost/variant.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;

#include <melosic/common/error.hpp>
#include <melosic/common/range.hpp>
#include "configvar.hpp"

class ConfigWidget;

namespace Melosic {
namespace Slots {
class Manager;
}
namespace Config {

class Base;

enum {
    CurrentDir,
    UserDir,
    SystemDir
};

static std::array<fs::path,3> dirs{{".", "~/.config/melosic/", "/etc/melosic/"}};

class Manager {
public:
    Manager(Slots::Manager&);
    ~Manager();

    void loadConfig();
    void saveConfig();

    Base& addConfigTree(const Base&);
    Base& getConfigRoot();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

class Base {
public:
    Base(const std::string& name);
    virtual ~Base() {}

    const std::string& getName() const;
    bool existsNode(const std::string& key) const;
    bool existsChild(const std::string& key) const;
    const VarType& getNode(const std::string& key) const;
    Base& getChild(const std::string& key);
    const Base& getChild(const std::string& key) const;
    Base& putChild(const std::string& key, const Base& child);
    const VarType& putNode(const std::string& key, const VarType& value);
    ForwardRange<Base* const> getChildren();
    ForwardRange<const Base* const> getChildren() const;
    ForwardRange<std::pair<const std::string, VarType> > getNodes();
    ForwardRange<const std::pair<const std::string, VarType> > getNodes() const;

    void addDefaultFunc(std::function<Base&()>);
    void resetToDefault();

    virtual ConfigWidget* createWidget();
    virtual QIcon* getIcon() const;

    virtual Base* clone() const {
        return new Base(*this);
    }

    template <typename T>
    T& get();

protected:
    Base();
    Base(const Base& b);
    Base& operator=(const Base& b);
    class impl;
    std::shared_ptr<impl> pimpl;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/);
};

Base* new_clone(const Base& conf);

template <typename T>
class Config : public Base {
public:
    virtual Base* clone() const override {
        return new T(*static_cast<const T*>(this));
    }

protected:
    Config(const std::string& name) : Base(name) {}
    Config() : Config("") {}

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & boost::serialization::base_object<Base>(*static_cast<T*>(this));
    }
};

} // namespace Config
} // namespace Melosic

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
extern template void Melosic::Config::Base::serialize<boost::archive::binary_iarchive>(
    boost::archive::binary_iarchive& ar,
    const unsigned int file_version
);
extern template void Melosic::Config::Base::serialize<boost::archive::binary_oarchive>(
    boost::archive::binary_oarchive& ar,
    const unsigned int file_version
);

#endif // MELOSIC_CONFIGURATION_HPP
