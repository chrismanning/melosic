#include <catch.hpp>

#include <iostream>
#include <experimental/optional>
#include <experimental/string_view>
#include <chrono>
using namespace std::literals;

#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <lastfm/service.hpp>
#include <lastfm/tag.hpp>
#include <lastfm/track.hpp>

#include <jbson/json_reader.hpp>

using namespace lastfm;

TEST_CASE("get_tag") {
    service serv{"47ee6adfdb3c68daeea2786add5e242d", "64a3811653376876431daad679ce5b67"};

    auto tag_fut = tag::get_info(serv, "Rock");

    auto status = tag_fut.wait_for(10000ms);
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
    std::clog << std::put_time(std::gmtime(&published), "%a %d %b %Y %H:%M:%S") << std::endl;

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
    service serv{"47ee6adfdb3c68daeea2786add5e242d", "64a3811653376876431daad679ce5b67"};

    auto artist_fut = artist::get_info(serv, "Metallica");

    auto status = artist_fut.wait_for(10000ms);
    REQUIRE(status == std::future_status::ready);

    auto artist = artist_fut.get();

    CHECK(artist.name() == "Metallica");
    CHECK(artist.url() == "http://www.last.fm/music/Metallica");

    auto wiki = artist.wiki();
    CHECK(wiki.summary().size() > 0);
    CHECK(wiki.content().size() > 0);
    CHECK(wiki.summary().size() <= wiki.content().size());

    auto published = date_t::clock::to_time_t(wiki.published());
    std::clog << std::put_time(std::gmtime(&published), "%a %d %b %Y %H:%M:%S") << std::endl;

    CHECK(artist.similar().size() > 0);
    CHECK(artist.top_tags().size() > 0);
}

TEST_CASE("get_artist_then") {
    service serv{"47ee6adfdb3c68daeea2786add5e242d", "64a3811653376876431daad679ce5b67"};

    auto artist_fut = artist::get_info(serv, "Metallica");

    serv.then(std::move(artist_fut), [](auto&& fut) {
        REQUIRE(fut.valid());
        auto status = fut.wait_for(0ms);
        REQUIRE(status == std::future_status::ready);

        auto artist = fut.get();

        CHECK(artist.name() == "Metallica");
        CHECK(artist.url() == "http://www.last.fm/music/Metallica");

        auto wiki = artist.wiki();
        CHECK(wiki.summary().size() > 0);
        CHECK(wiki.content().size() > 0);
        CHECK(wiki.summary().size() <= wiki.content().size());

        auto published = date_t::clock::to_time_t(wiki.published());
        std::clog << std::put_time(std::gmtime(&published), "%a %d %b %Y %H:%M:%S") << std::endl;

        CHECK(artist.similar().size() > 0);
        CHECK(artist.top_tags().size() > 0);
    }).wait();
}

TEST_CASE("get_track") {
    try {
    service serv{"47ee6adfdb3c68daeea2786add5e242d", "64a3811653376876431daad679ce5b67"};

    auto track_fut = track::get_info(serv, "Master of Puppets", "Metallica");

    auto status = track_fut.wait_for(10000ms);
    REQUIRE(status == std::future_status::ready);

    auto track = track_fut.get();

    CHECK(track.name() == "Master of Puppets");
    CHECK(track.url() == "http://www.last.fm/music/Metallica/_/Master+of+Puppets");

    auto wiki = track.wiki();
    CHECK(wiki.summary().size() > 0);
    CHECK(wiki.content().size() > 0);
    CHECK(wiki.summary().size() <= wiki.content().size());

    auto published = date_t::clock::to_time_t(wiki.published());
    std::clog << std::put_time(std::gmtime(&published), "%a %d %b %Y %H:%M:%S") << std::endl;

    CHECK(track.similar().size() == 0);
    CHECK(track.top_tags().size() > 0);
    }
    catch(...) {
        std::clog << boost::current_exception_diagnostic_information();
        CHECK(false);
    }
}
