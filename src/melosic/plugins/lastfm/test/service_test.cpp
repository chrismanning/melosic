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

TEST_CASE("get_tag") {
    lastfm::service serv{"47ee6adfdb3c68daeea2786add5e242d", "64a3811653376876431daad679ce5b67"};

    auto tag_fut = lastfm::tag::get_info(serv, "Rock");

    lastfm::tag tag;
    REQUIRE_NOTHROW(tag = tag_fut.get());

    CHECK(tag.name() == "rock");
    CHECK(tag.url() == "http://www.last.fm/tag/rock");
    CHECK(tag.reach() > 0);
    CHECK(tag.taggings() > 0);
    CHECK(tag.streamable());

    auto wiki = tag.wiki();
    CHECK(wiki.summary().size() > 0);
    CHECK(wiki.content().size() > 0);
    CHECK(wiki.summary().size() <= wiki.content().size());

    auto published = lastfm::date_t::clock::to_time_t(wiki.published());
    std::clog << std::put_time(std::gmtime(&published), "%a %d %b %Y %H:%M:%S") << std::endl;

    auto similar_tags_fut = tag.get_similar(serv);

    auto similar_tags = similar_tags_fut.get();
    CHECK(similar_tags.size() > 0);

    auto top_artists_fut = tag.get_top_artists(serv, 12);

    auto top_artists = top_artists_fut.get();
    CHECK(top_artists.size() == 12);
}

TEST_CASE("get_artist") {
    lastfm::service serv{"47ee6adfdb3c68daeea2786add5e242d", "64a3811653376876431daad679ce5b67"};

    auto artist_fut = lastfm::artist::get_info(serv, "Metallica");

    lastfm::artist artist;
    REQUIRE_NOTHROW(artist = artist_fut.get());

    CHECK(artist.name() == "Metallica");
    CHECK(artist.url() == "http://www.last.fm/music/Metallica");

    auto wiki = artist.wiki();
    CHECK(wiki.summary().size() > 0);
    CHECK(wiki.content().size() > 0);
    CHECK(wiki.summary().size() <= wiki.content().size());

    auto published = lastfm::date_t::clock::to_time_t(wiki.published());
    std::clog << std::put_time(std::gmtime(&published), "%a %d %b %Y %H:%M:%S") << std::endl;

    CHECK(artist.similar().size() > 0);
    CHECK(artist.top_tags().size() > 0);
}

TEST_CASE("get_artist_then") {
    lastfm::service serv{"47ee6adfdb3c68daeea2786add5e242d", "64a3811653376876431daad679ce5b67"};

    lastfm::artist::get_info(serv, "Metallica")
        .then([](pplx::task<lastfm::artist> artist_fut) {
            REQUIRE(artist_fut.is_done());
            lastfm::artist artist;
            REQUIRE_NOTHROW(artist = artist_fut.get());

            CHECK(artist.name() == "Metallica");
            CHECK(artist.url() == "http://www.last.fm/music/Metallica");

            auto wiki = artist.wiki();
            CHECK(wiki.summary().size() > 0);
            CHECK(wiki.content().size() > 0);
            CHECK(wiki.summary().size() <= wiki.content().size());

            auto published = lastfm::date_t::clock::to_time_t(wiki.published());
            std::clog << std::put_time(std::gmtime(&published), "%a %d %b %Y %H:%M:%S") << std::endl;

            CHECK(artist.similar().size() > 0);
            CHECK(artist.top_tags().size() > 0);
        })
        .wait();
}

TEST_CASE("get_track") {
    lastfm::service serv{"47ee6adfdb3c68daeea2786add5e242d", "64a3811653376876431daad679ce5b67"};

    auto track_fut = lastfm::track::get_info(serv, "Master of Puppets", "Metallica");

    lastfm::track track;
    REQUIRE_NOTHROW(track = track_fut.get());

    CHECK(track.name() == "Master of Puppets");
    CHECK(track.url() == "http://www.last.fm/music/Metallica/_/Master+of+Puppets");

    auto wiki = track.wiki();
    CHECK(wiki.summary().size() > 0);
    CHECK(wiki.content().size() > 0);
    CHECK(wiki.summary().size() <= wiki.content().size());

    auto published = lastfm::date_t::clock::to_time_t(wiki.published());
    std::clog << std::put_time(std::gmtime(&published), "%a %d %b %Y %H:%M:%S") << std::endl;

    CHECK(track.similar().size() == 0);
    CHECK(track.top_tags().size() > 0);
}
