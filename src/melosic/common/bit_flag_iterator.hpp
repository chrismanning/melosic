/**************************************************************************
**  Copyright (C) 2015 Christian Manning
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

#ifndef MELOSIC_BIT_FLAG_ITERATOR
#define MELOSIC_BIT_FLAG_ITERATOR

#include <bitset>
#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

namespace Melosic {

template <typename FlagT>
class bit_flag_iterator
    : public boost::iterator_facade<bit_flag_iterator<FlagT>, FlagT, boost::forward_traversal_tag, FlagT> {
    using underlying_type = std::underlying_type_t<typename bit_flag_iterator::value_type>;

    const std::bitset<sizeof(underlying_type) * 8> m_bits;
    int8_t m_pos = m_bits.size() + 1;

  public:
    bit_flag_iterator() noexcept = default;

    bit_flag_iterator(FlagT flag) noexcept : m_bits(static_cast<underlying_type>(flag)), m_pos(-1) {
        increment();
    }

  private:
    friend class boost::iterator_core_access;
    template <class> friend class bit_flag_iterator;

    template <typename T> bool equal(T&& other) const noexcept {
        return (m_pos >= static_cast<decltype(m_pos)>(m_bits.size()) &&
                other.m_pos >= static_cast<decltype(m_pos)>(m_bits.size())) ||
               (m_bits == other.m_bits && m_pos == other.m_pos);
    }

    auto dereference() const noexcept {
        return static_cast<typename bit_flag_iterator::reference>(1 << m_pos);
    }

    void increment() noexcept {
        while(++m_pos < static_cast<decltype(m_pos)>(m_bits.size())) {
            if(m_bits.test(m_pos))
                break;
        }
    }
};

template <typename FlagT> inline auto make_flag_range(FlagT flag) {
    using iterator_type = bit_flag_iterator<std::decay_t<FlagT>>;

    return boost::make_iterator_range(iterator_type{flag}, iterator_type{});
}

} // namespace Melosic

#endif // MELOSIC_BIT_FLAG_ITERATOR
