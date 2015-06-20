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

#ifndef LASTFM_IMAGE_HPP
#define LASTFM_IMAGE_HPP

#include <unordered_map>

#include <network/uri.hpp>

#include <lastfm/lastfm.hpp>

namespace lastfm {

enum class image_size { small, medium, large, extralarge, mega };

struct LASTFM_EXPORT image {
    explicit image() = default;

    const network::uri& uri() const;
    void uri(network::uri);

    image_size size() const;
    void size(image_size);

  private:
    network::uri m_uri;
    image_size m_size = image_size::small;
};

template <typename Container> void value_get(const jbson::basic_element<Container>& elem, image& var) {
    auto doc = jbson::get<jbson::element_type::document_element>(elem);
    for(auto&& elem : doc) {
        if(elem.name() == "#text") {
            var.uri(jbson::get<network::uri>(elem));
        } else if(elem.name() == "size") {
            var.size(jbson::get<image_size>(elem));
        }
    }
}

template <typename Container> void value_get(const jbson::basic_element<Container>& elem, image_size& var) {
    static const std::unordered_map<std::string_view, image_size> map = {{"small"_sv, image_size::small},
                                                                         {"medium"_sv, image_size::medium},
                                                                         {"large"_sv, image_size::large},
                                                                         {"extralarge"_sv, image_size::extralarge},
                                                                         {"mega"_sv, image_size::mega}};
    auto str = jbson::get<jbson::element_type::string_element>(elem);
    var = map.at(str);
}

} // namespace lastfm

#endif // LASTFM_IMAGE_HPP
