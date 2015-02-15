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

#ifndef LASTFM_ERROR
#define LASTFM_ERROR

#include <system_error>

#include <jbson/document.hpp>

namespace lastfm {

enum class api_error {
    unknown_error = 1,
    invalid_service = 2,
    invalid_method = 3,
    auth_failed = 4,
    invalid_format = 5,
    invalid_params = 6,
    invalid_resource = 7,
    operation_failed = 8,
    invalid_session_key = 9,
    invalid_api_key = 10,
    service_offline = 11,
    subsribers_only = 12,
    invalid_method_signature = 13,
    unauthorized_token = 14,
    item_not_streamable = 15,
    service_unavailable = 16,
    login_required = 17,
    trail_expired = 18,
    not_enough_content = 20,
    not_enough_members = 21,
    not_enough_fans = 22,
    not_enough_neighbours = 23,
    no_peak_radio = 24,
    radio_not_found = 25,
    api_key_suspended = 26,
    request_deprecated = 27,
    rate_limit_exceeded = 29,
};

std::error_code make_error_code(api_error err);

struct api_exception : std::system_error {
    explicit api_exception(api_error);
};

template <typename ContainerT>
[[noreturn]] inline void handle_error_response(const jbson::basic_element<ContainerT>& elem) {
    auto error = static_cast<api_error>(jbson::get<int>(elem));
    throw api_exception(error);
}

template <typename ContainerT, typename EContainerT>
inline void check_error(const jbson::basic_document<ContainerT, EContainerT>& doc) {
    for(auto&& elem : doc) {
        if(elem.name() == "error") {
            handle_error_response(elem);
        }
    }
}

} // namespace lastfm

namespace std {
template <> struct is_error_code_enum<lastfm::api_error> : public true_type {};
}

#endif // LASTFM_ERROR
