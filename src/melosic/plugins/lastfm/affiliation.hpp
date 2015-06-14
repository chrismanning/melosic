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

#ifndef LASTFM_AFFILIATION_HPP
#define LASTFM_AFFILIATION_HPP

#include <lastfm/lastfm.hpp>

namespace lastfm {

struct LASTFM_EXPORT affiliation {
    explicit affiliation() = default;

    std::string_view supplier_name() const;
    void supplier_name(std::string_view);

    const network::uri& buy_link() const;
    void buy_link(network::uri);

    std::string_view price() const;
    void price(std::string_view);

    const network::uri& supplier_icon() const;
    void supplier_icon(network::uri);

    bool is_search() const;
    void is_search(bool);

  private:
    std::string m_supplier_name;
    network::uri m_buy_link;
    std::string m_price;
    network::uri m_supplier_icon;
    bool m_is_search = false;
};

template <typename Container> void value_get(const jbson::basic_element<Container>& elem, affiliation& var) {
    auto doc = jbson::get<jbson::element_type::document_element>(elem);
    for(auto&& elem : doc) {
        if(elem.name() == "supplierName") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.supplier_name({str.data(), str.size()});
        } else if(elem.name() == "buyLink") {
            var.buy_link(jbson::get<network::uri>(elem));
        } else if(elem.name() == "price") {
            for(auto&& e : jbson::get<jbson::element_type::document_element>(elem)) {
                if(e.name() == "formatted") {
                    auto str = jbson::get<jbson::element_type::string_element>(elem);
                    var.price({str.data(), str.size()});
                }
            }
        } else if(elem.name() == "supplierIcon") {
            var.supplier_icon(jbson::get<network::uri>(elem));
        } else if(elem.name() == "isSearch") {
            var.is_search(jbson::get<jbson::element_type::string_element>(elem) == "1");
        }
    }
}

} // namespace lastfm

#endif // LASTFM_AFFILIATION_HPP
