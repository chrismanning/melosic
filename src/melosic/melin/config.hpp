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
    using KeyType = const std::string;
    using ChildType = Conf;
    using NodeType = VarType;
    using DefaultFunc = std::function<ChildType()>;

    Conf();
    explicit Conf(KeyType name);

    ~Conf();

    Conf(const Conf&);
    Conf(Conf&&);
    Conf& operator=(Conf);

    friend void swap(Conf&, Conf&) noexcept;

    typename std::add_const<KeyType>::type& getName() const noexcept;

    std::shared_ptr<std::pair<Conf::KeyType, VarType>> getNode(KeyType key);
    std::shared_ptr<const std::pair<Conf::KeyType, VarType>> getNode(KeyType key) const;
    std::shared_ptr<ChildType> getChild(KeyType key);
    std::shared_ptr<const ChildType> getChild(KeyType key) const;

    void putChild(Conf child);
    void putNode(KeyType key, VarType value);

    void removeChild(KeyType key);
    void removeNode(KeyType key);

    uint32_t childCount() const noexcept;
    uint32_t nodeCount() const noexcept;

    void iterateChildren(std::function<void(const ChildType&)>) const;
    void iterateChildren(std::function<void(ChildType&)>);
    void iterateNodes(std::function<void(const std::pair<Conf::KeyType, VarType>&)>) const;
    void iterateNodes(std::function<void(std::pair<Conf::KeyType, VarType>&)>);

    void merge(const Conf& c);

    template <typename Func>
    void addDefaultFunc(Func fun) {
        static_assert(std::is_same<decltype(fun()), Conf>::value, "Default function must return reference to Conf");
        addDefaultFunc(DefaultFunc(fun));
    }

    void addDefaultFunc(DefaultFunc);
    void resetToDefault();

    Signals::Config::VariableUpdated& getVariableUpdatedSignal() noexcept;

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

void swap(Conf& a, Conf& b) noexcept;

} // namespace Config
} // namespace Melosic

#endif // MELOSIC_CONFIGURATION_HPP
