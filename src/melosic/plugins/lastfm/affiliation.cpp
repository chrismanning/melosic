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

#include "affiliation.hpp"

namespace lastfm {

std::string_view affiliation::supplier_name() const {
    return m_supplier_name;
}

void affiliation::supplier_name(std::string_view supplier_name) {
    m_supplier_name = supplier_name.to_string();
}

const network::uri& affiliation::buy_link() const {
    return m_buy_link;
}

void affiliation::buy_link(network::uri buy_link) {
    m_buy_link = std::move(buy_link);
}

std::string_view affiliation::price() const {
    return m_price;
}

void affiliation::price(std::string_view price) {
    m_price = price.to_string();
}

const network::uri& affiliation::supplier_icon() const {
    return m_supplier_icon;
}

void affiliation::supplier_icon(network::uri supplier_icon) {
    m_supplier_icon = std::move(supplier_icon);
}

bool affiliation::is_search() const {
    return m_is_search;
}

void affiliation::is_search(bool is_search) {
    m_is_search = is_search;
}

} // namespace lastfm
