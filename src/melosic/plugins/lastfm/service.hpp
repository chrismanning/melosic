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

#ifndef LASTFM_SERVICE_HPP
#define LASTFM_SERVICE_HPP

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <future>
#include <thread>
#include <experimental/string_view>

#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/find_if.hpp>

#include <asio.hpp>
#include <asio/use_future.hpp>

#include <network/uri.hpp>
#include <network/http/v2/client.hpp>
#include <network/http/v2/client/response.hpp>

#include "lastfm.hpp"

namespace Melosic {

namespace Core {
class Track;
class Playlist;
}

namespace Output {
enum class DeviceState;
}
namespace Slots {
class Manager;
}
namespace Thread {
class Manager;
}
}

namespace lastfm {

struct user;
struct Method;
struct track;
struct tag;
struct artist;
using asio::use_future_t;
constexpr use_future_t<> use_future{};

class LASTFM_EXPORT service : public std::enable_shared_from_this<service> {
  public:
    using params_t = std::vector<std::tuple<std::string, std::string>>;

    service(std::string_view api_key, std::string_view shared_secret);

    ~service();

    //    Scrobbler scrobbler();
    user& getUser();
    void setUser(user u);

    std::string_view api_key() const;
    std::string_view shared_secret() const;

    std::shared_ptr<track> currentTrack();
    void trackChangedSlot(const Melosic::Core::Track& newTrack);
    void playlistChangeSlot(std::shared_ptr<Melosic::Core::Playlist> playlist);

    Method prepareMethodCall(const std::string& methodName);
    Method sign(Method method);
    std::string postMethod(const Method& method);

    network::http::v2::request make_read_request(std::string_view method, params_t params);
    network::http::v2::request make_write_request(std::string_view method, params_t params);

    std::future<network::http::v2::response> get(std::string_view method, params_t params, use_future_t<>);

    template <typename TransformerT, typename ReturnT = std::result_of_t<TransformerT(network::http::v2::response)>>
    std::future<ReturnT> get(std::string_view method, params_t params, use_future_t<>, TransformerT&& transform);

    void get(std::string_view method, params_t params,
             std::function<void(asio::error_code, network::http::v2::response)> callback);

    std::future<tag> get_tag(std::string_view tag_name);

    std::future<artist> get_artist(std::string_view artist_name);

  private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

template <typename TransformerT, typename ReturnT>
std::future<ReturnT> service::get(std::string_view method, params_t params, use_future_t<>, TransformerT&& transform) {
    using transformer_t = std::decay_t<TransformerT>;
    using ret_t = ReturnT;

    struct transformer_wrapper {
        void operator()(asio::error_code ec, network::http::v2::response res) const {
            if(ec) {
                m_promise->set_exception(std::make_exception_ptr(std::system_error(ec)));
                return;
            }

            try {
                m_promise->set_value(transform(res));
            } catch(...) {
                m_promise->set_exception(std::current_exception());
            }
        }

        transformer_t transform;
        mutable std::shared_ptr<std::promise<ret_t>> m_promise{std::make_shared<std::promise<ret_t>>()};
    } callback{std::forward<TransformerT>(transform)};

    auto fut = callback.m_promise->get_future();

    get(method, std::move(params), std::move(callback));

    return fut;
}

typedef std::map<std::string, std::string> StringStringMap;
typedef std::pair<std::string, std::string> Member;
struct Parameter {
    Parameter() = default;
    Parameter(Parameter&&) = delete;
    Parameter(const Parameter&) = delete;

    Parameter& addMember(const std::string& key, std::string_view value = {}) {
        members.emplace(key, value.data() ? value.to_string() : "");
        return *this;
    }

    boost::iterator_range<StringStringMap::iterator> getMembers() {
        return boost::make_iterator_range(members);
    }
    boost::iterator_range<StringStringMap::const_iterator> getMembers() const {
        return boost::make_iterator_range(members);
    }

  private:
    StringStringMap members;
};

typedef std::list<Parameter> ParameterList;

struct Method {
    Method(const std::string& methodName) : methodName(methodName) {
    }
    Method(Method&&) = default;

    Method(const Method&) = delete;
    Method& operator=(const Method&) = delete;

    Parameter& addParameter() {
        params.emplace_back();
        return params.back();
    }

    boost::iterator_range<ParameterList::iterator> getParameters() {
        return boost::make_iterator_range(params);
    }
    boost::iterator_range<ParameterList::const_iterator> getParameters() const {
        return boost::make_iterator_range(params);
    }

    const std::string& renderXML();

  private:
    friend class service;
    const std::string methodName;
    ParameterList params;
};
}

#endif // LASTFM_SERVICE_HPP
