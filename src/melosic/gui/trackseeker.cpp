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
#include <melosic/melin/sigslots/signals_fwd.hpp>
#include <melosic/melin/sigslots/signals.hpp>

#include "trackseeker.hpp"

class TrackSeeker::impl {
public:
    void onRelease() {
        seek(chrono::milliseconds(seekTo));
    }
    int seekTo;
    Signals::TrackSeeker::Seek seek;
};

TrackSeeker::TrackSeeker(QWidget *parent) :
    QSlider(parent),
    logject(boost::log::keywords::channel = "TrackSeeker"),
    pimpl(new impl)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
//    setGeometry(0,0,500,20);
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
//        std::clog << "Total: " << total.count();
//        std::clog << "; Maximum: " << maximum();
//        std::clog << "; Current: " << current.count();
//        std::clog << "; Value: " << value() << std::endl;
        setValue(current.count());
    }
}

void TrackSeeker::updateSeekTo(int s) {
    pimpl->seekTo = s >= this->maximum() ? this->maximum() - 1 : s;
}

void TrackSeeker::onRelease() {
    pimpl->onRelease();
}

template <>
Signals::TrackSeeker::Seek& TrackSeeker::get<Signals::TrackSeeker::Seek>() {
    return pimpl->seek;
}

template Signals::TrackSeeker::Seek& TrackSeeker::get<Signals::TrackSeeker::Seek>();
