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
#include <string>

#include <boost/filesystem/path.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/metafunctions.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/thread/synchronized_value.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <melosic/common/error.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/configvar.hpp>
#include <melosic/melin/config_signals.hpp>
#include <melosic/common/optional.hpp>

namespace Melosic {

namespace Slots {
class Manager;
}

namespace Config {

class Conf;

enum { CurrentDir, UserDir, SystemDir };

class MELOSIC_EXPORT Manager {
  public:
    explicit Manager(boost::filesystem::path p);
    ~Manager();

    void loadConfig();
    void saveConfig();

    boost::synchronized_value<Conf>& getConfigRoot();

    Signals::Config::Loaded& getLoadedSignal() const;

  private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

class MELOSIC_EXPORT Conf final : boost::partially_ordered<Conf>,
                                  boost::equality_comparable<Conf>,
                                  public boost::intrusive_ref_counter<Conf, boost::thread_safe_counter> {
  public:
    using conf_variable_type = VarType;
    using node_key_type = std::string;
    using node_mapped_type = conf_variable_type;

    using child_key_type = std::string;
    using child_value_type = boost::intrusive_ptr<Conf>;
    using child_const_value_type = boost::intrusive_ptr<const Conf>;

    Conf();
    explicit Conf(std::string name);

    ~Conf();

    Conf(const Conf&);
    Conf(Conf&&);
    Conf& operator=(const Conf&)&;
    Conf& operator=(Conf&&)&;

    void swap(Conf&) noexcept;

    const std::string& name() const;

    //! Get child which may or may not exist.
    child_value_type getChild(const child_key_type& key);
    //! \overload
    child_const_value_type getChild(const child_key_type& key) const;

    //! Get child which may or may not exist. Create it if it does not.
    child_value_type createChild(const child_key_type& key);
    //! Get child which may or may not exist. Create it with a specific value if it does not.
    child_value_type createChild(const child_key_type& key, const child_value_type& def);
    //! \overload
    child_value_type createChild(const child_key_type& key, child_value_type&& def);
    //! \overload
    child_value_type createChild(const child_key_type& key, const Conf& def);
    //! \overload
    child_value_type createChild(const child_key_type& key, Conf&& def);

    //! Add a child Conf. Overwrites child with same name.
    child_value_type putChild(const child_value_type& child);
    //! \overload
    child_value_type putChild(child_value_type&& child);
    //! \overload
    child_value_type putChild(const Conf& child);
    //! \overload
    child_value_type putChild(Conf&& child);

    //! Get node which may or may not exist.
    optional<node_mapped_type> getNode(const node_key_type& key) const;

    //! Get node which may or may not exist. Create it if it does not.
    node_mapped_type createNode(const node_key_type& key);
    //! Get node which may or may not exist. Create it with a specific value if it does not.
    node_mapped_type createNode(const node_key_type& key, const node_mapped_type& def);
    //! \overload
    node_mapped_type createNode(const node_key_type& key, node_mapped_type&& def);

    //! Add a node value. Overwrites node with same key.
    void putNode(const node_key_type& key, const node_mapped_type& value);
    //! \overload
    void putNode(const node_key_type& key, node_mapped_type&& value);

    //! \overload
    //! value is VarType compatible, but not VarType.
    template <typename T>
    void putNode(const node_key_type& key, T&& value,
                 std::enable_if_t<boost::mpl::contains<VarType::types, std::decay_t<T>>::value>* = 0) {
        putNode(key, VarType(std::forward<T>(value)));
    }

    //! \overload
    //! value is range of VarType compatible, but not VarType compatible itself.
    template <typename Range>
    void putNode(
        const node_key_type& key, Range&& range,
        std::enable_if_t<!boost::mpl::contains<VarType::types, std::decay_t<Range>>::value>* = 0,
        std::enable_if_t<boost::has_range_const_iterator<std::decay_t<Range>>::value>* = 0,
        std::enable_if_t<
            boost::mpl::contains<VarType::types, typename boost::range_value<std::decay_t<Range>>::type>::value>* = 0) {
        if(boost::empty(range)) {
            putNode(key, std::vector<VarType>{});
            return;
        }

        std::vector<VarType> varRange{boost::distance(range)};
        boost::copy(range, varRange.begin());
        putNode(key, VarType(std::move(varRange)));
    }

    //! \overload
    //! value is range of filesystem::path.
    template <typename Range>
    void putNode(const node_key_type& key, Range&& range,
                 std::enable_if_t<boost::has_range_const_iterator<std::decay_t<Range>>::type::value>* = 0,
                 std::enable_if_t<std::is_same<typename boost::range_value<std::decay_t<Range>>::type,
                                               boost::filesystem::path>::value>* = 0) {
        if(boost::empty(range)) {
            putNode(key, std::vector<VarType>{});
            return;
        }
        using boost::adaptors::transformed;
        std::vector<Config::VarType> varRange{boost::distance(range)};
        boost::copy(range | transformed([](boost::filesystem::path p) { return p.string(); }), varRange.begin());
        putNode(key, VarType(std::move(varRange)));
    }

    void removeChild(const child_key_type& key);
    void removeNode(const node_key_type& key);

    uint32_t childCount() const noexcept;
    uint32_t nodeCount() const noexcept;

    void iterateChildren(std::function<void(child_const_value_type)>) const;
    void iterateNodes(std::function<void(const node_key_type&, const node_mapped_type&)>) const;

    void merge(const Conf& c);

    void setDefault(Conf);
    void resetToDefault();

    Signals::Config::VariableUpdated& getVariableUpdatedSignal() noexcept;

    bool operator<(const Conf&) const;
    bool operator==(const Conf&) const;

  private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

void swap(Conf& a, Conf& b) noexcept;

} // namespace Config
} // namespace Melosic

#endif // MELOSIC_CONFIGURATION_HPP
