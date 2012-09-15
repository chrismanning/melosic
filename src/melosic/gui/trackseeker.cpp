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

#include <melosic/managers/output/pluginterface.hpp>
#include "trackseeker.hpp"

TrackSeeker::TrackSeeker(QWidget *parent) : QSlider(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
//    setGeometry(0,0,500,20);
    setEnabled(false);
    setMinimum(0);
    setMaximum(0);
    setTracking(false);

    connect(this, SIGNAL(sliderMoved(int)), this, SLOT(updateSeekTo(int)));
    connect(this, SIGNAL(sliderReleased()), this, SLOT(onRelease()));
}

TrackSeeker::~TrackSeeker() {
}

void TrackSeeker::onStateChangeSlot(DeviceState state) {
    switch(state) {
        case DeviceState::Playing:
            std::cerr << "TrackSeeker Playing\n";
            if(!isEnabled()) {
                setEnabled(true);
            }
            break;
        case DeviceState::Error:
            std::cerr << "TrackSeeker Error\n";
        case DeviceState::Stopped:
            std::cerr << "TrackSeeker Stopped\n";
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

void TrackSeeker::onNotifySlot(std::chrono::milliseconds current, std::chrono::milliseconds total) {
    if(!isSliderDown()) {
        setRange(0, total.count());
//        std::clog << "Total: " << total.count();
//        std::clog << "; Maximum: " << maximum();
//        std::clog << "; Current: " << current.count();
//        std::clog << "; Value: " << value() << std::endl;
        setValue(current.count());
    }
}
