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
#include <boost/signals2.hpp>

#include <melosic/common/logging.hpp>

namespace Melosic {
namespace Output {
enum class DeviceState;
}
}
using namespace Melosic;
using Output::DeviceState;

class TrackSeeker : public QSlider {
    Q_OBJECT
public:
    explicit TrackSeeker(QWidget *parent = 0);
    ~TrackSeeker();

    void onStateChangeSlot(DeviceState state);
    void onNotifySlot(std::chrono::milliseconds current, std::chrono::milliseconds total);

    typedef boost::signals2::signal<void(std::chrono::milliseconds)> SeekSignal;
    boost::signals2::connection connectSeek(const SeekSignal::slot_type& slot) {
        return seek.connect(slot);
    }

private Q_SLOTS:
    void updateSeekTo(int s) { seekTo = s >= this->maximum() ? this->maximum() - 1 : s;}
    void onRelease() {
        seek(std::chrono::milliseconds(seekTo));
    }

private:
    Logger::Logger logject;
    int seekTo;
    SeekSignal seek;
};

#endif // MELOSIC_TRACKSEEKER_HPP
