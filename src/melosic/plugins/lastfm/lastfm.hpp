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

#ifndef LASTFM_HPP
#define LASTFM_HPP

#include <QtCore>

#include <lastfm/Audioscrobbler.h>
#include <lastfm/Track.h>

#include <boost/chrono.hpp>
namespace chrono = boost::chrono;
#include <boost/shared_ptr.hpp>
#include <boost/signals2/connection.hpp>

namespace Melosic {
class Track;
class Playlist;
namespace Output {
enum class DeviceState;
}
}
using namespace Melosic;

class Scrobbler : QObject {
    Q_OBJECT
public:
    Scrobbler();

    void notifySlot(chrono::milliseconds current, chrono::milliseconds total);
    void trackChangedSlot(const Track& newTrack);
    void stateChangedSlot(Output::DeviceState state);
    void playlistChangeSlot(boost::shared_ptr<Playlist> playlist);

private Q_SLOTS:
    void printError(int code, QString msg);

private:
    lastfm::Audioscrobbler as;
    boost::shared_ptr<lastfm::Track> currentTrack;
};

#endif // LASTFM_HPP
