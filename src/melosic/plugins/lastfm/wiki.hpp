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

#ifndef LASTFM_WIKI_HPP
#define LASTFM_WIKI_HPP

#include <string>
#include <experimental/string_view>

#include <jbson/element.hpp>

#include "lastfm.hpp"

namespace lastfm {

struct LASTFM_EXPORT wiki {
    explicit wiki(std::string_view summary = {}, std::string_view content = {}, date_t published = {});

    std::string_view summary() const;
    void summary(std::string_view);

    std::string_view content() const;
    void content(std::string_view);

    date_t published() const;
    void published(date_t);

  private:
    std::string m_summary;
    std::string m_content;
    date_t m_published;
};

template <typename Container> void value_get(const jbson::basic_element<Container>& elem, wiki& var) {
    auto doc = jbson::get<jbson::element_type::document_element>(elem);
    for(auto&& elem : doc) {
        if(elem.name() == "summary") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.summary({str.data(), str.size()});
        } else if(elem.name() == "content") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.content({str.data(), str.size()});
        } else if(elem.name() == "published") {
            var.published(jbson::get<date_t>(elem));
        }
    }
}

} // namespace lastfm

#endif // LASTFM_WIKI_HPP
