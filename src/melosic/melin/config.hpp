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
#include <boost/range/concepts.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/metafunctions.hpp>
#include <boost/mpl/contains.hpp>

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

    std::tuple<Conf&, std::unique_lock<std::mutex>&&> getConfigRoot();

    Signals::Config::Loaded& getLoadedSignal() const;

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

class MELOSIC_EXPORT Conf final {
public:
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

    std::shared_ptr<std::pair<KeyType, VarType>> getNode(std::add_const<KeyType>::type& key);
    std::shared_ptr<const std::pair<KeyType, VarType>> getNode(std::add_const<KeyType>::type& key) const;
    std::shared_ptr<ChildType> getChild(std::add_const<KeyType>::type& key);
    std::shared_ptr<const ChildType> getChild(std::add_const<KeyType>::type& key) const;

    void putChild(Conf child);
    void putNode(KeyType key, VarType value);

    template <typename T>
    void putNode(KeyType key, const T& value,
                 typename std::enable_if<boost::mpl::contains<VarType::types, T>::value>::type* = 0) {
        putNode(std::move(key), VarType(value));
    }

    template <typename Range>
    void putNode(KeyType key, const Range& range,
                 typename std::enable_if<!boost::mpl::contains<VarType::types, Range>::value>::type* = 0,
                 typename std::enable_if<boost::has_range_const_iterator<Range>::value>::type* = 0,
                 typename std::enable_if<boost::mpl::contains<VarType::types,
                                               typename boost::range_value<Range>::type>::value>::type* = 0)
    {
        if(boost::empty(range))
            return;
        std::vector<VarType> varRange{boost::distance(range)};
        boost::copy(range, varRange.begin());
        putNode(std::move(key), VarType(varRange));
    }

    template <typename Range>
    void putNode(KeyType key, const Range& range,
                 typename std::enable_if<boost::has_range_const_iterator<Range>::type::value>::type* = 0,
                 typename std::enable_if<std::is_same<typename boost::range_value<Range>::type,
                                         boost::filesystem::path>::value>::type* = 0)
    {
        if(boost::empty(range))
            return;
        using boost::adaptors::transformed;
        std::vector<Config::VarType> varRange{boost::distance(range)};
        boost::copy(range | transformed([] (boost::filesystem::path p) { return p.string(); }), varRange.begin());
        putNode(std::move(key), VarType(std::move(varRange)));
    }

    void removeChild(std::add_const<KeyType>::type& key);
    void removeNode(std::add_const<KeyType>::type& key);

    uint32_t childCount() const noexcept;
    uint32_t nodeCount() const noexcept;

    void iterateChildren(std::function<void(const ChildType&)>) const;
    void iterateChildren(std::function<void(ChildType&)>);
    void iterateNodes(std::function<void(const std::pair<std::add_const<KeyType>::type, VarType>&)>) const;
    void iterateNodes(std::function<void(std::pair<std::add_const<KeyType>::type, VarType>&)>);

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
