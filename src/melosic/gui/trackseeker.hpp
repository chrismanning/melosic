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

#ifndef MELOSIC_TRACKSEEKER_HPP
#define MELOSIC_TRACKSEEKER_HPP

#include <QSlider>
#include <QHBoxLayout>
#include <chrono>
namespace chrono = std::chrono;
#include <memory>

#include <melosic/melin/logging.hpp>

namespace Melosic {
namespace Output {
enum class DeviceState;
}

namespace Slots {
class Manager;
}
}
using namespace Melosic;
using Output::DeviceState;

class TrackSeeker : public QSlider {
    Q_OBJECT
    class impl;
    Logger::Logger logject;
    std::shared_ptr<impl> pimpl;
public:
    explicit TrackSeeker(QWidget* parent = 0);
    ~TrackSeeker();

    void onStateChangeSlot(DeviceState state);
    void onNotifySlot(chrono::milliseconds current, chrono::milliseconds total);

    void connectSlots(Slots::Manager*);

    template <typename T>
    T& get();

private Q_SLOTS:
    void updateSeekTo(int s);
    void onRelease();
};

#endif // MELOSIC_TRACKSEEKER_HPP
