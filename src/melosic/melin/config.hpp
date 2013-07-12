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

#include <memory>
#include <array>

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;

#include <melosic/common/error.hpp>
#include <melosic/common/range.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/configvar.hpp>
#include <melosic/melin/config_signals.hpp>

namespace Melosic {

class ConfigWidget;

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
    Manager();
    ~Manager();

    MELOSIC_EXPORT void loadConfig();
    MELOSIC_EXPORT void saveConfig();

    MELOSIC_EXPORT Base& addConfigTree(const Base&);
    MELOSIC_EXPORT Base& getConfigRoot();

    MELOSIC_EXPORT Signals::Config::Loaded& getLoadedSignal() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

class MELOSIC_EXPORT Base {
public:
    Base(const std::string& name);
    virtual ~Base();

    const std::string& getName() const;
    bool existsNode(const std::string& key) const;
    bool existsChild(const std::string& key) const;
    const VarType& getNode(const std::string& key) const;
    Base& getChild(const std::string& key);
    const Base& getChild(const std::string& key) const;
    Base& putChild(const std::string& key, const Base& child);
    const VarType& putNode(const std::string& key, const VarType& value);
    ForwardRange<std::shared_ptr<Base>> getChildren();
    ForwardRange<const std::shared_ptr<Melosic::Config::Base>> getChildren() const;
    ForwardRange<std::pair<const std::string, VarType>> getNodes();
    ForwardRange<const std::pair<const std::string, VarType>> getNodes() const;

    void addDefaultFunc(std::function<Base&()>);
    void resetToDefault();

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
    std::unique_ptr<impl> pimpl;

    friend class boost::serialization::access;
    template<class Archive>
    MELOSIC_EXPORT void serialize(Archive& ar, const unsigned int /*version*/);
};

Base* new_clone(const Base& conf);

template <typename T>
class Config : public Base {
public:
    virtual ~Config() {}
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
extern template MELOSIC_EXPORT void Melosic::Config::Base::serialize<boost::archive::binary_iarchive>(
    boost::archive::binary_iarchive& ar,
    const unsigned int file_version
);
extern template MELOSIC_EXPORT void Melosic::Config::Base::serialize<boost::archive::binary_oarchive>(
    boost::archive::binary_oarchive& ar,
    const unsigned int file_version
);

namespace boost {
namespace serialization {

template<class Archive, class T>
inline void save(Archive& ar,
                 const std::shared_ptr<T>& t,
                 const unsigned int /* file_version */)
{
    // The most common cause of trapping here would be serializing
    // something like shared_ptr<int>.  This occurs because int
    // is never tracked by default.  Wrap int in a trackable type
    BOOST_STATIC_ASSERT((tracking_level<T>::value != track_never));
    const T* t_ptr = t.get();
    ar << boost::serialization::make_nvp("px", t_ptr);
}

template<class Archive, class T>
inline void load(Archive& ar,
                 std::shared_ptr<T>& t,
                 const unsigned int /*file_version*/)
{
    // The most common cause of trapping here would be serializing
    // something like shared_ptr<int>.  This occurs because int
    // is never tracked by default.  Wrap int in a trackable type
    BOOST_STATIC_ASSERT((tracking_level<T>::value != track_never));
    T* r;
    ar >> boost::serialization::make_nvp("px", r);
    t.reset(r);
//    ar.reset(t,r);
}

template<class Archive, class T>
inline void serialize(Archive& ar,
                      std::shared_ptr<T>& t,
                      const unsigned int file_version)
{
    // correct shared_ptr serialization depends upon object tracking
    // being used.
    BOOST_STATIC_ASSERT(boost::serialization::tracking_level<T>::value
                        != boost::serialization::track_never
    );
    boost::serialization::split_free(ar, t, file_version);
}

} // namespace serialization
} // namespace boost

#endif // MELOSIC_CONFIGURATION_HPP
