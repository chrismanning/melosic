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

#include <memory>

#include <openssl/md5.h>

#include <cpprest/http_client.h>

#include <boost/hana/ext/std.hpp>

#include <jbson/document.hpp>
#include <jbson/json_reader.hpp>

#include <lastfm/service.hpp>
#include <lastfm/user.hpp>
#include <lastfm/error.hpp>

namespace lastfm {

static constexpr auto base_url = U("http://ws.audioscrobbler.com/2.0/"_sv);

struct service::impl {
    impl(std::string_view api_key, std::string_view shared_secret)
        : cas_client(web::uri{base_url.to_string()}), api_key(api_key), shared_secret(shared_secret) {
        assert(!api_key.empty());
        assert(!shared_secret.empty());
    }

    struct convert_to_casablanca_string_impl {
        template <typename S> decltype(auto) operator()(S&& in) const {
#ifdef _WIN32
            thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.from_bytes(in.begin(), in.end());
#else
            return std::forward<S>(in);
#endif
        }
    };
    static constexpr convert_to_casablanca_string_impl convert_to_casablanca_string{};

    pplx::task<std::string> get(std::string_view method, params_t params) {
        web::uri_builder uri_build;

        uri_build.append_query(U("method"), convert_to_casablanca_string(method));
        for(auto&& member : params) {
            hana::unpack(hana::transform(std::move(member), convert_to_casablanca_string),
                         [&](auto&& a, auto&& b) { uri_build.append_query(std::move(a), std::move(b), true); });
        }
        uri_build.append_query(U("format"), U("json"));
        uri_build.append_query(U("api_key"), convert_to_casablanca_string(api_key));

        return cas_client.request(web::http::methods::GET, uri_build.to_string())
            .then([](web::http::http_response response) { return response.extract_utf8string(true); });
    }

    web::http::client::http_client cas_client;

    const std::string api_key;
    const std::string shared_secret;
    user user;
    friend class service;
};

service::service(std::string_view api_key, std::string_view shared_secret)
    : pimpl(std::make_unique<impl>(api_key, shared_secret)) {
}

service::~service() {
}

std::string_view service::api_key() const {
    return pimpl->api_key;
}

std::string_view service::shared_secret() const {
    return pimpl->shared_secret;
}

// Method service::sign(Method method) {
//    TRACE_LOG(logject) << "Signing method call";
//    method.getParameters().front().addMember("sk", getUser().getSessionKey());
//    std::deque<Member> members(method.getParameters().front().getMembers().begin(),
//                               method.getParameters().front().getMembers().end());
//    members.emplace_back("method", method.methodName);
//    sort(members, [](const Member& a, const Member& b) { return b.first > a.first; });
//    std::string sig;
//    for(auto& mem : members) {
//        sig += mem.first + mem.second;
//    }
//    sig += shared_secret().to_string();
//    TRACE_LOG(logject) << "sig: " << sig;

//    std::array<uint8_t, MD5_DIGEST_LENGTH> sigp;
//    MD5(reinterpret_cast<const uint8_t*>(sig.c_str()), sig.length(), sigp.data());
//    sig.clear();

//    hex(sigp, std::back_inserter(sig));
//    TRACE_LOG(logject) << "MD5 sig: " << sig;

//    method.getParameters().front().addMember("api_sig", sig);

//    return std::move(method);
//}

pplx::task<jbson::document> service::get(std::string_view method, service::params_t params) {
    return pimpl->get(method, std::move(params))
        .then([](std::string body) {
            auto doc = jbson::read_json(body);
            detail::check_error(doc);
            return doc;
        });
}

} // namespace lastfm
