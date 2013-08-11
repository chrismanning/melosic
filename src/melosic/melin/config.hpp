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
#include <string>

#include <boost/filesystem/path.hpp>
#include <boost/range/sub_range.hpp>
#include <boost/variant.hpp>
#include <boost/range/adaptors.hpp>
namespace {
using namespace boost::adaptors;
}

#include <melosic/common/error.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/configvar.hpp>
#include <melosic/melin/config_signals.hpp>

namespace Melosic {

namespace Slots {
class Manager;
}

namespace Config {

class Conf;

enum {
    CurrentDir,
    UserDir,
    SystemDir
};

class MELOSIC_EXPORT Manager {
public:
    explicit Manager(boost::filesystem::path p);
    ~Manager();

    void loadConfig();
    void saveConfig();

    Signals::Config::Loaded& getLoadedSignal() const;

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

class MELOSIC_EXPORT Conf final {
public:
    typedef std::map<std::string, Conf> ChildMap;
    typedef boost::range_detail::select_second_mutable_range<ChildMap> ChildRange;
    typedef boost::range_detail::select_second_const_range<ChildMap> ConstChildRange;
    typedef std::map<std::string, VarType> NodeMap;
    typedef boost::sub_range<NodeMap> NodeRange;
    typedef boost::sub_range<const NodeMap> ConstNodeRange;
    typedef std::function<Conf()> DefaultFunc;

    Conf();
    explicit Conf(std::string name);

    ~Conf();

    Conf(const Conf&);
    Conf(Conf&&);
    Conf& operator=(Conf);

    friend void swap(Conf&, Conf&) noexcept;

    const std::string& getName() const;
    bool existsNode(std::string key) const;
    bool existsChild(std::string key) const;
    const VarType& getNode(std::string key) const;
    Conf& getChild(std::string key);
    const Conf& getChild(std::string key) const;
    Conf& putChild(std::string key, Conf child);
    VarType& putNode(std::string key, VarType value);
    ChildRange getChildren();
    ConstChildRange getChildren() const;
    NodeRange getNodes();
    ConstNodeRange getNodes() const;

    Conf& merge(const Conf& c);

    template <typename Func>
    void addDefaultFunc(Func fun) {
        static_assert(std::is_same<decltype(fun()), Conf>::value, "Default function must return reference to Conf");
        addDefaultFunc(DefaultFunc(fun));
    }

    void addDefaultFunc(DefaultFunc);
    void resetToDefault();

    Signals::Config::VariableUpdated& getVariableUpdatedSignal();

private:
    struct impl;
    std::unique_ptr<impl> pimpl;

    friend class Manager;
};

void swap(Conf& a, Conf& b) noexcept;

} // namespace Config
} // namespace Melosic

#endif // MELOSIC_CONFIGURATION_HPP
