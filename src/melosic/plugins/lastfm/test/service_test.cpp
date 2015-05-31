#include <catch.hpp>

#include <iostream>
#include <experimental/optional>
#include <experimental/string_view>
#include <chrono>
using namespace std::literals;

#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "../service.hpp"
#include "../tag.hpp"

#include <jbson/json_reader.hpp>

using namespace lastfm;

TEST_CASE("get_tag") {
    service serv{"47ee6adfdb3c68daeea2786add5e242d", "64a3811653376876431daad679ce5b67"};

    auto tag_fut = serv.get_tag("Rock");

    auto status = tag_fut.wait_for(1000ms);
    REQUIRE(status == std::future_status::ready);

    auto tag = tag_fut.get();

    CHECK(tag.name() == "rock");
    CHECK(tag.url() == "http://www.last.fm/tag/rock");
    CHECK(tag.reach() > 0);
    CHECK(tag.taggings() > 0);
    CHECK(tag.streamable());

    auto wiki = tag.wiki();
    CHECK(wiki.summary().size() > 0);
    CHECK(wiki.content().size() > 0);
    CHECK(wiki.summary().size() <= wiki.content().size());

    auto published = date_t::clock::to_time_t(wiki.published());
    auto tm = std::localtime(&published);
    std::clog << std::put_time(tm, "%a %d %b %Y %H:%M:%S") << std::endl;

    auto similar_tags_fut = tag.get_similar(serv);
    status = similar_tags_fut.wait_for(5000ms);
    REQUIRE(status == std::future_status::ready);

    auto similar_tags = similar_tags_fut.get();
    CHECK(similar_tags.size() > 0);

    auto top_artists_fut = tag.get_top_artists(serv, 12);
    status = top_artists_fut.wait_for(5000ms);
    REQUIRE(status == std::future_status::ready);

    auto top_artists = top_artists_fut.get();
    CHECK(top_artists.size() == 12);
}

TEST_CASE("get_artist") {
    try {
    service serv{"47ee6adfdb3c68daeea2786add5e242d", "64a3811653376876431daad679ce5b67"};

    auto artist_fut = serv.get_artist("Metallica");

    auto status = artist_fut.wait_for(1000ms);
    REQUIRE(status == std::future_status::ready);

    auto artist = artist_fut.get();

    CHECK(artist.name() == "Metallica");
    CHECK(artist.url() == "http://www.last.fm/music/Metallica");
    CHECK(artist.streamable());

    auto wiki = artist.wiki();
    CHECK(wiki.summary().size() > 0);
    CHECK(wiki.content().size() > 0);
    CHECK(wiki.summary().size() <= wiki.content().size());

    auto published = date_t::clock::to_time_t(wiki.published());
    auto tm = std::localtime(&published);
    std::clog << std::put_time(tm, "%a %d %b %Y %H:%M:%S") << std::endl;

    CHECK(artist.similar().size() > 0);
    CHECK(artist.tags().size() > 0);
    }
    catch(...) {
        std::cout << boost::current_exception_diagnostic_information();
        CHECK(false);
    }
}
