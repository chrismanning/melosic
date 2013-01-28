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

//#include <QIcon>
class QIcon;

#include <memory>
#include <map>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/serialize_ptr_map.hpp>
#include <boost/signals2.hpp>
#include <boost/variant.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/range/iterator_range.hpp>

#include <melosic/common/error.hpp>

class ConfigWidget;

namespace Melosic {

class Configuration {
public:
    typedef boost::variant<std::string, bool, int64_t, double> VarType;
    typedef boost::signals2::signal<void(const std::string&, const VarType&)> VarUpdateSignal;
    typedef boost::ptr_map<std::string, Configuration> ChildMap;
    typedef std::map<std::string, VarType> NodeMap;

    Configuration(const std::string& name);
    Configuration() : Configuration("") {}

    const std::string& getName() const;
    bool existsNode(const std::string& key);
    bool existsChild(const std::string& key);
    const VarType& getNode(const std::string& key);
    Configuration& getChild(const std::string& key);
    Configuration& putChild(const std::string& key, const Configuration& child);
    const VarType& putNode(const std::string& key, const VarType& value);
    boost::signals2::connection connectVariableUpdateSlot(const VarUpdateSignal::slot_type& slot);
    boost::iterator_range<ChildMap::iterator> getChildren() {
        return boost::make_iterator_range(children);
    }
    boost::iterator_range<ChildMap::const_iterator> getChildren() const {
        return boost::make_iterator_range(children);
    }

    virtual ConfigWidget* createWidget();
    virtual QIcon* getIcon() const;

    virtual Configuration* clone() const {
        return new Configuration(*this);
    }

protected:
    Configuration(const Configuration& b);
    Configuration& operator=(const Configuration& b);
    ChildMap children;
    NodeMap nodes;
    VarUpdateSignal variableUpdated;
    std::string name;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & children;
        ar & nodes;
        ar & name;
    }
};

inline Melosic::Configuration* new_clone(const Melosic::Configuration& conf) {
    return conf.clone();
}

} //end namespace Melosic

#endif // MELOSIC_CONFIGURATION_HPP
