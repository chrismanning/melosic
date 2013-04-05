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

#include <melosic/melin/output.hpp>
#include <melosic/melin/sigslots/slots.hpp>
#include <melosic/melin/sigslots/signals_fwd.hpp>
#include <melosic/melin/sigslots/signals.hpp>

#include "trackseeker.hpp"

namespace Melosic {

class TrackSeeker::impl {
public:
    void onRelease() {
        seek(chrono::milliseconds(seekTo));
    }
    Signals::Player::Seek seek;
    int seekTo = 0;
    std::list<Signals::ScopedConnection> scopedSigConns;
};

TrackSeeker::TrackSeeker(QWidget *parent) :
    QSlider(parent),
    logject(logging::keywords::channel = "TrackSeeker"),
    pimpl(new impl)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    setMinimumWidth(50);
    setEnabled(false);
    setMinimum(0);
    setMaximum(0);
    setTracking(false);

    connect(this, &TrackSeeker::sliderMoved, this, &TrackSeeker::updateSeekTo);
    connect(this, &TrackSeeker::sliderReleased, this, &TrackSeeker::onRelease);
}

TrackSeeker::~TrackSeeker() {}

void TrackSeeker::onStateChangeSlot(DeviceState state) {
    switch(state) {
        case DeviceState::Playing:
            TRACE_LOG(logject) << "TrackSeeker Playing";
            if(!isEnabled()) {
                setEnabled(true);
            }
            break;
        case DeviceState::Error:
            TRACE_LOG(logject) << "TrackSeeker Error";
        case DeviceState::Stopped:
            TRACE_LOG(logject) << "TrackSeeker Stopped";
//            if(isEnabled()) {
//                setEnabled(false);
//            }
            setMaximum(0);
            break;
        case DeviceState::Paused:
            break;
        case DeviceState::Ready:
        default:
            break;
    }
}

void TrackSeeker::onNotifySlot(chrono::milliseconds current, chrono::milliseconds total) {
    if(!isSliderDown()) {
        setRange(0, total.count());
        setValue(current.count());
    }
}

void TrackSeeker::connectSlots(Slots::Manager* slotman) {
    slotman->get<Signals::Player::Seek>().connect(pimpl->seek);
    pimpl->scopedSigConns.emplace_back(slotman->get<Signals::Player::StateChanged>()
                                       .emplace_connect(&TrackSeeker::onStateChangeSlot, this, ph::_1));
    pimpl->scopedSigConns.emplace_back(slotman->get<Signals::Player::NotifyPlayPos>()
                                       .emplace_connect(&TrackSeeker::onNotifySlot, this, ph::_1, ph::_2));
}

void TrackSeeker::updateSeekTo(int s) {
    pimpl->seekTo = s >= this->maximum() ? this->maximum() - 1 : s;
}

void TrackSeeker::onRelease() {
    pimpl->onRelease();
}

template <>
Signals::Player::Seek& TrackSeeker::get<Signals::Player::Seek>() {
    return pimpl->seek;
}

template Signals::Player::Seek& TrackSeeker::get<Signals::Player::Seek>();

} // namespace Melosic
