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

#ifndef LASTFM_EVENT_HPP
#define LASTFM_EVENT_HPP

#include <lastfm/lastfm.hpp>
#include <lastfm/venue.hpp>
#include <lastfm/image.hpp>

namespace lastfm {

struct artist;

struct LASTFM_EXPORT event {
    explicit event() = default;

    std::string_view id() const;
    void id(std::string_view id);

    std::string_view name() const;
    void name(std::string_view name);

    const std::vector<artist>& artists() const;
    void artists(std::vector<artist> artists);

    const venue& venue() const;
    void venue(struct venue venue);

    date_t start_date() const;
    void start_date(date_t start_date);

    std::string_view description() const;
    void description(std::string_view description);

    const std::vector<image>& images() const;
    void images(std::vector<image> images);

    int attendance() const;
    void attendance(int attendance);

    const network::uri& url() const;
    void url(network::uri url);

  private:
    std::string m_id;
    std::string m_name;
    std::vector<artist> m_artists;
    struct venue m_venue;
    date_t m_start_date;
    std::string m_description;
    std::vector<image> m_images;
    int m_attendance = 0;
    network::uri m_url;
};

template <typename Container> void value_get(const jbson::basic_element<Container>& event_elem, event& var) {
    auto doc = jbson::get<jbson::element_type::document_element>(event_elem);
    for(auto&& elem : doc) {
        if(elem.name() == "id") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.id({str.data(), str.size()});
        } else if(elem.name() == "title") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.name({str.data(), str.size()});
        } else if(elem.name() == "url") {
            var.url(jbson::get<network::uri>(elem));
        } else if(elem.name() == "attendance") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.attendance(std::strtol(str.data(), nullptr, 10));
        } else if(elem.name() == "description") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.description(str);
        } else if(elem.name() == "startDate") {
            var.start_date(jbson::get<date_t>(elem));
        } else if(elem.name() == "venue") {
            var.venue(jbson::get<venue>(elem));
        } else if(elem.name() == "artists") {
            if(elem.type() == jbson::element_type::document_element) {
                for(auto&& e : jbson::get<jbson::element_type::document_element>(elem))
                    if(e.name() == "artist")
                        var.artists(jbson::get<std::vector<artist>>(e));
            } else {
                var.artists(jbson::get<std::vector<artist>>(elem));
            }
        } else if(elem.name() == "image") {
            var.images(jbson::get<std::vector<image>>(elem));
        }
    }
}

} // namespace lastfm

#endif // LASTFM_EVENT_HPP
