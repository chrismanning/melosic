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

#include <lastfm/ws.h>
#include "lastfm.hpp"

#include <melosic/common/exports.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/common/logging.hpp>
#include <melosic/managers/output/pluginterface.hpp>
#include <melosic/core/configuration.hpp>
using namespace Melosic;

static Logger::Logger logject(boost::log::keywords::channel = "LastFM");

static const Plugin::Info lastFmInfo{"LastFM",
                                     Plugin::Type::utility | Plugin::Type::service | Plugin::Type::gui,
                                     Plugin::generateVersion(1,0,0),
                                     Plugin::expectedAPIVersion(),
                                     ::time_when_compiled()
                                    };

Scrobbler::Scrobbler() : as("tst") {
    connect(&as, SIGNAL(nowPlayingError(int,QString)), this, SLOT(printError(int,QString)));
}

void Scrobbler::notifySlot(chrono::milliseconds current, chrono::milliseconds total) {
    if(total > chrono::seconds(15) && current > (total / 2) && bool(currentTrack)) {
        TRACE_LOG(logject) << "Caching track for submission";
        as.cache(*currentTrack);
    }
}

void Scrobbler::trackChangedSlot(const Track& newTrack) {
    currentTrack.reset();
    TRACE_LOG(logject) << "Submitting cache";
    as.submit();

    lastfm::MutableTrack ltrack;
    ltrack.stamp();
    {   auto artist = newTrack.getTag("artist");
        if(artist != "?")
            ltrack.setArtist(QString::fromStdString(artist));}
    {   auto title = newTrack.getTag("title");
        if(title != "?")
            ltrack.setTitle(QString::fromStdString(title));}
    {   auto album = newTrack.getTag("album");
        if(album != "?")
            ltrack.setAlbum(QString::fromStdString(album));}
    {   auto tracknum = newTrack.getTag("tracknumber");
        if(tracknum != "?")
            ltrack.setTrackNumber(QString::fromStdString(tracknum).toInt());}
    ltrack.setDuration(chrono::duration_cast<chrono::seconds>(newTrack.duration()).count());

    currentTrack.reset(new lastfm::Track(ltrack));
}

void Scrobbler::stateChangedSlot(DeviceState state) {
    if(state == DeviceState::Stopped) {
        currentTrack.reset();
        as.submit();
    }
    if(state == DeviceState::Playing && bool(currentTrack)) {
        as.nowPlaying(*currentTrack);
    }
}

static boost::shared_ptr<Scrobbler> scrobbler;
static std::list<boost::signals2::scoped_connection> connections;
static Configuration conf;

void Scrobbler::playlistChangeSlot(boost::shared_ptr<Playlist> playlist) {
    connections.emplace_back(
                playlist->connectTrackChangedSlot(
                    Playlist::TrackChangedSignal::slot_type(&Scrobbler::trackChangedSlot,
                                                            this,
                                                            _1).track(scrobbler)));
    if(playlist->current() != playlist->end())
        trackChangedSlot(*playlist->current());
}

void Scrobbler::printError(int code, QString msg) {
    ERROR_LOG(logject) << code << ": " << msg.toStdString();
}

void refreshConfig(const std::string& key, const std::string& value) {
    if(key == "username") {
        lastfm::ws::Username = QString::fromStdString(value);
    }
    else if(key == "scrobbling") {
        if(value == "true") {
            scrobbler.reset(new Scrobbler);

            connections.emplace_back(
                        Kernel::getPlayer().connectNotifySlot(
                            Player::NotifySignal::slot_type(&Scrobbler::notifySlot,
                                                            scrobbler.get(),
                                                            _1,
                                                            _2).track(scrobbler)));
            connections.emplace_back(
                        Kernel::getPlayer().connectStateSlot(
                            Player::StateSignal::slot_type(&Scrobbler::stateChangedSlot,
                                                           scrobbler.get(),
                                                           _1).track(scrobbler)));
            connections.emplace_back(
                        Kernel::getPlayer().connectPlaylistChangeSlot(
                            Player::PlaylistChangeSignal::slot_type(&Scrobbler::playlistChangeSlot,
                                                                    scrobbler.get(),
                                                                    _1).track(scrobbler)));
        }
        else {
            connections.clear();
            scrobbler.reset();
        }
    }
}

extern "C" void registerPlugin(Plugin::Info* info) {
    *info = ::lastFmInfo;

    lastfm::ws::ApiKey = "47ee6adfdb3c68daeea2786add5e242d";
    lastfm::ws::SharedSecret = "64a3811653376876431daad679ce5b67";

    ::conf.putNode("username", "");
    ::conf.putNode("sessionkey", "");
    ::conf.putNode("scrobbling", "false");
    Kernel::getConfig().putChild("lastfm", ::conf).connectVariableUpdateSlot(refreshConfig);
}

extern "C" void destroyPlugin() {}
