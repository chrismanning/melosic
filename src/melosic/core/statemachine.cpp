/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#include <functional>
#include <mutex>
using std::mutex;
using lock_guard = std::lock_guard<mutex>;
using unique_lock = std::unique_lock<mutex>;
#include <thread>
namespace this_thread = std::this_thread;

#include <melosic/melin/sigslots/signals.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/common/smart_ptr_equality.hpp>
#include <melosic/melin/logging.hpp>
#include "statemachine.hpp"

namespace Melosic {
namespace Core {

struct StateChanged : Signals::Signal<void(Output::DeviceState)> {
    using super::Signal;
};

struct State {
    explicit State(StateChanged& stateChanged) : stateChanged(stateChanged) {}
    virtual ~State() {}

    //default actions (do nothing)
    virtual void play() {}
    virtual void pause() {}
    virtual void stop() {}

    virtual chrono::milliseconds tell() const {
        return chrono::milliseconds::zero();
    }

//    virtual std::function<void()> save();

    virtual Output::DeviceState state() const {
        return Output::DeviceState::Initial;
    }

    virtual void sinkChange();

private:
    static StateMachine::impl* stateMachine_;
    static Melosic::Playlist::Manager* playman_;
    static void setPlayer(StateMachine::impl* p) { stateMachine_ = p; }
    static void setPlayMan(Melosic::Playlist::Manager* p) { playman_ = p; }
    friend class StateMachine::impl;

protected:
    decltype(stateMachine_) const& stateMachine = stateMachine_;
    decltype(playman_) const& playman = playman_;
    StateChanged& stateChanged;
};
StateMachine::impl* State::stateMachine_(nullptr);
Melosic::Playlist::Manager* State::playman_(nullptr);

struct Stopped;

class StateMachine::impl {
public:
    explicit impl(Melosic::Playlist::Manager& playman, Output::Manager& outman) :
        playman(playman),
        outman(outman),
        logject(logging::keywords::channel = "StateMachine"),
        currentState_(new State(stateChanged))
    {
        State::stateMachine_ = this;
        State::playman_ = &playman;
    }

    void play() {
        unique_lock l(mu);
        auto cs = currentState_;
        assert(cs);
        l.unlock();
        cs->play();
    }

    void pause() {
        unique_lock l(mu);
        auto cs = currentState_;
        assert(cs);
        l.unlock();
        cs->pause();
    }

    void stop() {
        unique_lock l(mu);
        auto cs = currentState_;
        assert(cs);
        l.unlock();
        cs->stop();
    }

    chrono::milliseconds tell() {
        unique_lock l(mu);
        auto cs = currentState_;
        assert(cs);
        l.unlock();
        return cs->tell();
    }

    Output::DeviceState state() {
        lock_guard l(mu);
        auto cs = currentState_;
        assert(cs);
//        l.unlock();
//        cs
//        lock_guard l(mu);
        assert(cs);
//        assert(currentState_->device());
//        return currentState_->device()->state();
        return cs->state();
    }

    Output::PlayerSink& sink() {
        return sinkWrapper_;
    }

    void trackChangeSlot(const Track& track) {
        auto cp = playman.currentPlaylist();
        assert(cp);
        try {
            if(*cp && sinkWrapper_) {
                assert(track == *cp->currentTrack());
                if(sinkWrapper_.currentSpecs() != track.getAudioSpecs()) {
                    LOG(logject) << "Changing sink specs for new track";
                    sinkWrapper_.stop();
                    this_thread::sleep_for(chrono::milliseconds(100));
                    sinkWrapper_.prepareSink(const_cast<Track&>(track).getAudioSpecs());
                    this_thread::sleep_for(chrono::milliseconds(100));
                    sinkWrapper_.play();
                }
            }
        }
        catch(...) {
            ERROR_LOG(logject) << boost::diagnostic_information(boost::current_exception());
            playman.currentPlaylist()->next();
        }
    }

    void changeDevice() {
        lock_guard l(sinkWrapper_.mu);
        sinkWrapper_.device = std::move(outman.getPlayerSink());
    }

    void sinkChangeSlot() {
        try {
            unique_lock l(mu);
            auto cs = currentState_;
            assert(cs);
            l.unlock();
            cs->sinkChange();
        }
        catch(...) {
            ERROR_LOG(logject) << boost::diagnostic_information(boost::current_exception());

            stop();
        }
    }

    Melosic::Playlist::Manager& playman;
    struct SinkWrapper : Output::PlayerSink {
        std::streamsize write(const char* s, std::streamsize n) override {
            lock_guard l(mu);
            assert(device);
            return device->write(s, n);
        }
        const std::string& getSinkName() override {
            lock_guard l(mu);
            assert(device);
            return device->getSinkName();
        }
        void prepareSink(Melosic::AudioSpecs& as) override {
            lock_guard l(mu);
            assert(device);
            device->prepareSink(as);
        }
        const Melosic::AudioSpecs& currentSpecs() override {
            lock_guard l(mu);
            assert(device);
            return device->currentSpecs();
        }
        const std::string& getSinkDescription() override {
            lock_guard l(mu);
            assert(device);
            return device->getSinkDescription();
        }
        void play() override {
            lock_guard l(mu);
            assert(device);
            device->play();
        }
        void pause() override {
            lock_guard l(mu);
            assert(device);
            device->pause();
        }
        void stop() override {
            lock_guard l(mu);
            assert(device);
            device->stop();
        }
        Output::DeviceState state() override {
            lock_guard l(mu);
            assert(device);
            return device->state();
        }

        explicit operator bool() {
            return static_cast<bool>(device);
        }

        mutex mu;

        std::unique_ptr<PlayerSink> device;
    } sinkWrapper_;

    Output::Manager& outman;
    Logger::Logger logject;
    mutex mu;

    std::function<void()> restoreFun;

    std::shared_ptr<State> currentState_;
    friend class StateMachine;
    friend struct State;

    StateChanged stateChanged;

    template <typename S>
    auto changeState() -> decltype(currentState_) {
        static_assert(std::is_base_of<State, S>::value, "Template parameter must be State derived");

        lock_guard l(mu);

        auto r(currentState_);
        assert(r);
        currentState_.reset(new S(r->stateChanged));
        return std::move(r);
    }
};

struct Stopped;

void State::sinkChange() {
    stateMachine_->changeDevice();
    stateMachine->changeState<Stopped>();
}

struct Stop : virtual State {
    explicit Stop(StateChanged& sc) : State(sc) {}
    virtual void stop() override {
        assert(stateMachine);
        stateMachine->sink().stop();
        auto ptr = stateMachine->changeState<Stopped>();
        assert(ptr == this);
    }
};

struct Tell : virtual State {
    explicit Tell(StateChanged& sc) : State(sc) {}
    virtual chrono::milliseconds tell() const override {
        assert(playman);
        assert(playman->currentPlaylist());
        auto playlist = playman->currentPlaylist();
        assert(playlist);
        if(playlist->currentTrack() == playlist->end())
            return chrono::milliseconds::zero();
        return playlist->currentTrack()->tell();
    }
};

struct Paused;

struct Playing : virtual State, Stop, Tell {
    explicit Playing(StateChanged& sc) : State(sc), Stop(sc), Tell(sc) {
        assert(stateMachine->sinkWrapper_);
        stateChanged(Output::DeviceState::Playing);
    }

    void pause() override {
        assert(stateMachine);
        stateMachine->sink().pause();
        auto ptr = stateMachine->changeState<Paused>();
        assert(ptr == this);
    }

    Output::DeviceState state() const override {
        assert(stateMachine);
        assert(stateMachine->sink().state()==Output::DeviceState::Playing);
        return Output::DeviceState::Playing;
    }

    void sinkChange() override {
        auto time(tell());
        stateMachine->sink().stop();
        stateMachine->changeDevice();
        stateMachine->play();
        stateMachine->playman.currentPlaylist()->currentTrack()->seek(time);
    }
};

struct Paused : virtual State, Stop, Tell {
    explicit Paused(StateChanged& sc) : State(sc), Stop(sc), Tell(sc) {
        assert(stateMachine->sinkWrapper_);
        stateChanged(Output::DeviceState::Paused);
    }

    void play() override {
        assert(stateMachine);
        stateMachine->sink().play();
        auto ptr = stateMachine->changeState<Playing>();
        assert(ptr == this);
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Paused;
    }

    void sinkChange() override {
        auto time(tell());
        stateMachine->sink().stop();
        stateMachine->changeDevice();
        stateMachine->play();
        stateMachine->playman.currentPlaylist()->currentTrack()->seek(time);
        stateMachine->pause();
    }
};

struct Stopped : State {
    explicit Stopped(StateChanged& sc) : State(sc) {
        assert(stateMachine->sinkWrapper_);
        stateChanged(Output::DeviceState::Stopped);
    }

    void play() override {
        assert(stateMachine);
        assert(playman);
        assert(playman->currentPlaylist());
        if(!*playman->currentPlaylist())
            return;
        try {
            stateMachine->sink().prepareSink(playman->currentPlaylist()->currentTrack()->getAudioSpecs());
            stateMachine->sink().play();
            auto ptr = stateMachine->changeState<Playing>();
            assert(ptr == this);
        }
        catch(...) {
            ERROR_LOG(stateMachine->logject) << boost::diagnostic_information(boost::current_exception());
        }
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Stopped;
    }
};

StateMachine::StateMachine(Melosic::Playlist::Manager& playman, Output::Manager& outman) :
    pimpl(new impl(playman, outman)) {}

StateMachine::~StateMachine() {}

void StateMachine::play() {
    pimpl->play();
}

void StateMachine::pause() {
    pimpl->pause();
}

void StateMachine::stop() {
    pimpl->stop();
}

chrono::milliseconds StateMachine::tell() const {
    return pimpl->tell();
}

Output::DeviceState StateMachine::state() const {
    return pimpl->state();
}

Output::PlayerSink& StateMachine::sink() {
    return pimpl->sinkWrapper_;
}

void StateMachine::sinkChangeSlot() {
    pimpl->sinkChangeSlot();
}

void StateMachine::trackChangeSlot(const Track& track) {
    pimpl->trackChangeSlot(track);
}

} // namespace Core
} // namespace Melosic
