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

#include <string>
using namespace std::string_literals;

#include "./error.hpp"

namespace lastfm {

class error_category : public std::error_category {
  public:
    //! Default constructor.
    constexpr error_category() noexcept = default;

    //! Returns name of category (`"EJDB"`).
    const char* name() const noexcept override {
        return "EJDB";
    }

    //! Returns message associated with \p ecode.
    std::string message(int ecode) const noexcept override {
        switch(static_cast<api_error>(ecode)) {
            case api_error::invalid_service:
                return "Invalid service -This service does not exist"s;
            case api_error::invalid_method:
                return "Invalid Method - No method with that name in this package"s;
            case api_error::auth_failed:
                return "Authentication Failed - You do not have permissions to access the service"s;
            case api_error::invalid_format:
                return "Invalid format - This service doesn't exist in that format"s;
            case api_error::invalid_params:
                return "Invalid parameters - Your request is missing a required parameter"s;
            case api_error::invalid_resource:
                return "Invalid resource specified"s;
            case api_error::operation_failed:
                return "Operation failed - Most likely the backend service failed. Please try again."s;
            case api_error::invalid_session_key:
                return "Invalid session key - Please re-authenticate"s;
            case api_error::invalid_api_key:
                return "Invalid API key - You must be granted a valid key by last.fm"s;
            case api_error::service_offline:
                return "Service Offline - This service is temporarily offline. Try again later."s;
            case api_error::subsribers_only:
                return "Subscribers Only - This station is only available to paid last.fm subscribers"s;
            case api_error::invalid_method_signature:
                return "Invalid method signature supplied"s;
            case api_error::unauthorized_token:
                return "Unauthorized Token - This token has not been authorized"s;
            case api_error::item_not_streamable:
                return "This item is not available for streaming."s;
            case api_error::service_unavailable:
                return "The service is temporarily unavailable, please try again."s;
            case api_error::login_required:
                return "Login: User requires to be logged in"s;
            case api_error::trail_expired:
                return "Trial Expired - This user has no free radio plays left. Subscription required."s;
            case api_error::not_enough_content:
                return "Not Enough Content - There is not enough content to play this station"s;
            case api_error::not_enough_members:
                return "Not Enough Members - This group does not have enough members for radio"s;
            case api_error::not_enough_fans:
                return "Not Enough Fans - This artist does not have enough fans for for radio"s;
            case api_error::not_enough_neighbours:
                return "Not Enough Neighbours - There are not enough neighbours for radio"s;
            case api_error::no_peak_radio:
                return "No Peak Radio - This user is not allowed to listen to radio during peak usage"s;
            case api_error::radio_not_found:
                return "Radio Not Found - Radio station not found"s;
            case api_error::api_key_suspended:
                return "API Key Suspended - This application is not allowed to make requests to the web services"s;
            case api_error::request_deprecated:
                return "Deprecated - This type of request is no longer supported"s;
            case api_error::rate_limit_exceeded:
                return "Rate Limit Exceded - "
                       "Your IP has made too many requests in a short period, exceeding our API guidelines"s;
            default:
                return "This error does not exist"s;
        }
    }
};

std::error_code make_error_code(api_error err) {
    static const error_category category{};
    return std::error_code(static_cast<int>(err), category);
}

api_exception::api_exception(api_error err) : std::system_error(make_error_code(err)) {
}

} // namespace lastfm
