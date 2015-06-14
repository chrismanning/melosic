#include <catch.hpp>

#include <iostream>
#include <experimental/optional>
#include <experimental/string_view>
namespace std {
using namespace experimental;
}

#include <lastfm/http_client.hpp>

#include <jbson/json_reader.hpp>
#include <jbson/path.hpp>

using namespace network::http::v0;

TEST_CASE("Get") {
    asio::io_service io;
    std::unique_ptr<asio::io_service::work> null_work{new asio::io_service::work(io)};
    client cli{io};
    asio::thread t([&]() { io.run(); });

    request req{network::uri{"http://ws.audioscrobbler.com/2.0/"
                             "?method=artist.getsimilar&artist=Metallica&limit=5"
                             "&api_key=47ee6adfdb3c68daeea2786add5e242d&format=json"}};
    req.version("1.1").append_header("User-Agent", "cpp-netlib client_test")
        //      .method(method::get)
        /*.append_header("Connection", "close")*/;

    auto fut = cli.get(req, {}, use_future);

    null_work.reset();

    response res;
    REQUIRE_NOTHROW(res = fut.get());

    CHECK(res.status() == status::code::ok);
    CHECK(res.status_message() == "OK");
    CHECK(res.version() == "HTTP/1.0");

    auto headers = res.headers();
    CHECK(std::begin(headers)->first == "Server");

    for(auto&& header : headers) {
        std::clog << header.first << ": " << header.second << std::endl;
    }

    std::clog << res.body() << std::endl;

    t.join();
}

static std::optional<std::string> get_header(const response& res, std::string_view name) {
    for(auto&& header : res.headers()) {
        if(name == header.first)
            return header.second;
    }
    return std::nullopt;
}

TEST_CASE("Get Transform") {
    asio::io_service io;
    std::unique_ptr<asio::io_service::work> null_work{new asio::io_service::work(io)};
    client cli{io};
    asio::thread t([&]() { io.run(); });

    request req{network::uri{"http://ws.audioscrobbler.com/2.0/"
                             "?method=artist.getsimilar&artist=Metallica&limit=5"
                             "&api_key=47ee6adfdb3c68daeea2786add5e242d&format=json"}};
    req.version("1.1").method(method::get).append_header("User-Agent", "cpp-netlib client_test")
        /*.append_header("Connection", "close")*/;

    auto fut = cli.execute(req, {}, use_future, [=](response res) {
        REQUIRE(res.status() == status::code::ok);
        const auto content_type_opt = get_header(res, "Content-Type");
        REQUIRE(content_type_opt);
        CHECK(content_type_opt->size() == ::strlen("application/json; charset=utf-8;"));
        auto content_type = boost::trim_copy(*content_type_opt);
        REQUIRE(content_type == "application/json; charset=utf-8;");

        auto doc = jbson::read_json(res.body());

        auto elems = jbson::path_select(doc, "$.similarartists.artist[*].name");
        CHECK(elems.size() == 5);

        std::vector<std::string> names;

        std::transform(elems.begin(), elems.end(), std::back_inserter(names),
                       [](auto&& element) { return jbson::get<std::string>(element); });

        return names;
    });

    null_work.reset();

    std::vector<std::string> names;
    REQUIRE_NOTHROW(names = fut.get());

    CHECK(names.size() == 5);

    for(auto&& name : names) {
        std::clog << name << std::endl;
    }

    t.join();
}
