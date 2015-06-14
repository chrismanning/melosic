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

#include <asio/package.hpp>
#include <asio/post.hpp>
#include <asio/use_future.hpp>

#include <network/uri.hpp>
#include <network/http/v2/client.hpp>
#include <network/http/v2/client/response.hpp>

#include <jbson/document.hpp>

#include <lastfm/lastfm.hpp>
#include <lastfm/detail/hana_optional.hpp>
#include <lastfm/detail/transform.hpp>

namespace hana = boost::hana;

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

    template <typename TransformerT, typename ReturnT = std::result_of_t<TransformerT(jbson::document)>>
    std::future<ReturnT> get(std::string_view method, params_t params, use_future_t<>, TransformerT&& transform);

    void get(std::string_view method, params_t params,
             std::function<void(asio::error_code, network::http::v2::response)> callback);

    template <typename T, typename ContinueT>
    auto then(std::future<T> fut, ContinueT&& continuation);

  private:
    static jbson::document document_callback(asio::error_code, network::http::v2::response);

    struct impl;
    std::unique_ptr<impl> pimpl;
};

template <typename T, typename ContinueT>
auto service::then(std::future<T> fut, ContinueT&& continuation) {
    return asio::post(asio::package([fut=std::move(fut), continuation=std::forward<ContinueT>(continuation)]() mutable {
        fut.wait();
        return continuation(std::move(fut));
    }));
}

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
                auto doc = document_callback(ec, std::move(res));

                m_promise->set_value(transform(doc));
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

namespace detail {

struct to_string_ {
    inline std::string operator()(date_t t) const {
        auto time = date_t::clock::to_time_t(t);
        std::stringstream ss{};
        ss << std::put_time(std::gmtime(&time), "%a, %d %b %Y %H:%M:%S");
        return ss.str();
    }
    inline std::string operator()(std::string_view s) const { return s.to_string(); }
    inline std::string operator()(std::string s) const { return std::move(s); }
    template <typename N>
    inline std::string operator()(N&& i) const { return ::std::to_string(i); }
};

struct make_params_ {
    template <typename... PairT> service::params_t operator()(PairT&&... optional_params) const {
        service::params_t params;
        for(auto&& param : {make_param(optional_params)...}) {
            auto& name = std::get<0>(param);
            auto& maybe = std::get<1>(param);
            if(maybe && !maybe->empty())
                params.emplace_back(name, *maybe);
        }
        return params;
    }
private:
    template <typename PairT> static decltype(auto) make_param(PairT&& t) {
        using std::get;
        constexpr auto to_string = to_string_{};
        return std::make_tuple(get<0>(t), hana::transform(hana::flatten(std::make_optional(get<1>(t))), to_string));
    }
};

} // namespace detail

constexpr auto make_params = detail::make_params_{};

} // namespace lastfm

#endif // LASTFM_SERVICE_HPP
