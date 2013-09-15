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

#include <array>
#include <thread>
#include <mutex>
namespace this_thread = std::this_thread;
using std::thread; using std::mutex;
using unique_lock = std::unique_lock<mutex>;
using lock_guard = std::lock_guard<mutex>;
#include <atomic>
#include <functional>

#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>
namespace io = boost::iostreams;
#include <boost/scope_exit.hpp>
#include <boost/asio.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/variant.hpp>

#include <melosic/common/error.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/common/range.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/common/smart_ptr_equality.hpp>
#include <melosic/asio/audio_io.hpp>
#include <melosic/melin/kernel.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/common/int_get.hpp>

#include "player.hpp"

namespace Melosic {
namespace Core {

struct Stopped;

struct NotifyPlayPos : Signals::Signal<Signals::Player::NotifyPlayPos> {
    using Super::Signal;
};

struct StateChanged : Signals::Signal<Signals::Player::StateChanged> {
    using Super::Signal;
};

struct State;

struct Player::impl : std::enable_shared_from_this<Player::impl> {
    explicit impl(Core::Kernel& kernel);

    ~impl();

    void play() {
        unique_lock l(mu);
        play_impl();
        if(state_impl() == DeviceState::Playing)
            write_handler(boost::system::error_code(), buffer_size);
    }
    void play_impl();

    void pause() {
        unique_lock l(mu);
        pause_impl();
    }
    void pause_impl();

    void stop() {
        unique_lock l(mu);
        stop_impl();
        for(auto& buf : in_buf)
            ::operator delete(ASIO::buffer_cast<void*>(buf));
        in_buf.clear();
    }
    void stop_impl();

    void seek(chrono::milliseconds dur);

    chrono::milliseconds tell() {
        unique_lock l(mu);
        return tell_impl();
    }
    chrono::milliseconds tell_impl();

    Output::DeviceState state() {
        unique_lock l(mu);
        return state_impl();
    }
    Output::DeviceState state_impl();

    void trackChangeSlot(const Track& track);
    void changeDevice();
    void sinkChangeSlot();

    Melosic::Playlist::Manager& playman;
    Output::Manager& outman;
    Config::Manager& confman;

    mutex mu;

    StateChanged stateChanged;

    std::shared_ptr<State> currentState_;
    friend struct State;

    NotifyPlayPos notifyPlayPosition;
    std::atomic_bool end;
    thread playerThread;
    Logger::Logger logject{logging::keywords::channel = "Player"};
    friend class Player;

    template <typename S>
    auto changeState() -> decltype(currentState_) {
        static_assert(std::is_base_of<State, S>::value, "Template parameter must be State derived");

        TRACE_LOG(logject) << "changeState(): changing state to " << typeid(S);

        auto r(currentState_);
        assert(r);
        currentState_.reset(new S(stateChanged));
        assert(currentState_);
        return std::move(r);
    }

    std::unique_ptr<ASIO::AudioOutputBase> asioOutput;

    std::list<ASIO::mutable_buffer> in_buf;

    void write_handler(boost::system::error_code ec, std::size_t n) {
        TRACE_LOG(logject) << "write_handler: " << n << " bytes written";
        if(ec) {
            ERROR_LOG(logject) << "write error: " << ec.message();
            if(ec.value() != boost::system::errc::operation_canceled)
                stop();
            else
                stop_impl();
            return;
        }

        n = buffer_size;
        ASIO::mutable_buffer tmp{::operator new(n), n};
        in_buf.emplace_back(tmp);
        auto playlist = playman.currentPlaylist();

        if(!playlist)
            return;

        try {
            auto n_ = playlist->read(ASIO::buffer_cast<char*>(tmp), n);
            if(n_ > 0 && static_cast<std::size_t>(n_) < n)
                ec = ASIO::error::make_error_code(ASIO::error::eof);

            if(n_ <= 0) {
                stop();
                playlist->jumpTo(0);
                return;
            }
        }
        catch(...) {
            ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
            stop_impl();
            playman.currentPlaylist()->next();
            play_impl();
        }

        read_handler(ec, n);
    }

    void read_handler(boost::system::error_code ec, std::size_t n) {
        TRACE_LOG(logject) << "read_handler: " << n << " bytes read";
        if(ec || in_buf.empty())
            return;

        ASIO::mutable_buffer tmp{std::move(in_buf.front())};
        in_buf.pop_front();
        ASIO::async_write(*asioOutput, ASIO::buffer(tmp),
        [this, tmp] (boost::system::error_code ec, std::size_t n) {
            if(n < ASIO::buffer_size(tmp)) {
                if(ec.value() != ASIO::error::eof) {
                    ASIO::mutable_buffer tmp2;
                    ASIO::buffer_copy(tmp2, tmp + n);
                    in_buf.push_front(tmp2);
                }
                ::operator delete(ASIO::buffer_cast<void*>(tmp));
                ec.clear();
            }
            if(n == 0 || ec)
                return;

            write_handler(ec, n);
        });
    }

    Config::Conf conf{"Player"};
    std::size_t buffer_size = 8192;

    void loadedSlot(Config::Conf& base) {
        TRACE_LOG(logject) << "Player conf loaded";

        auto c = base.getChild("Player");
        if(!c) {
            base.putChild(conf);
            c = base.getChild("Player");
        }
        assert(c);
        c->merge(conf);
        c->addDefaultFunc([=]() -> Config::Conf { return conf; });
        c->iterateNodes([&] (const std::pair<Config::KeyType, Config::VarType>& pair) {
            TRACE_LOG(logject) << "Config: variable loaded: " << pair.first;
            variableUpdateSlot(pair.first, pair.second);
        });
        c->getVariableUpdatedSignal().connect(&impl::variableUpdateSlot, this);
    }

    void variableUpdateSlot(const Config::KeyType& key, const Config::VarType& val) {
        using boost::get;
        using Config::get;
        TRACE_LOG(logject) << "Config: variable updated: " << key;
        try {
            if(key == "buffer size") {
                buffer_size = get<size_t>(val);
            }
            else
                ERROR_LOG(logject) << "Config: Unknown key: " << key;
        }
        catch(boost::bad_get&) {
            ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
        }
    }
};

struct State {
protected:
    explicit State(StateChanged& stateChanged) : stateChanged(stateChanged) {}

public:
    virtual ~State() {}

    //default actions (do nothing)
    virtual void play() {}
    virtual void pause() {}
    virtual void stop() {}

    virtual chrono::milliseconds tell() const {
        return 0ms;
    }

    virtual Output::DeviceState state() const = 0;

    virtual void sinkChange() {}

    static Player::impl* stateMachine;
    static Melosic::Playlist::Manager* playman;

    StateChanged& stateChanged;
};
decltype(State::stateMachine) State::stateMachine(nullptr);
decltype(State::playman) State::playman(nullptr);

struct Error;
struct Playing;

struct Stopped : State {
    explicit Stopped(StateChanged& sc) : State(sc) {
        stateChanged(Output::DeviceState::Stopped);
    }

    void play() override {
        assert(playman->currentPlaylist());
        if(!*playman->currentPlaylist())
            return;

        try {
            stateMachine->changeDevice();
            if(!stateMachine->asioOutput)
                BOOST_THROW_EXCEPTION(std::exception());
            stateMachine->asioOutput->prepare(playman->currentPlaylist()->currentTrack()->getAudioSpecs());
            stateMachine->asioOutput->play();
            auto ptr = stateMachine->changeState<Playing>();
            assert(ptr == this);
        }
        catch(...) {
            ERROR_LOG(stateMachine->logject) << boost::current_exception_diagnostic_information();
            stateMachine->changeState<Error>();
        }
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Stopped;
    }
};

struct Error : State {
    explicit Error(StateChanged& sc) : State(sc) {
        stateChanged(Output::DeviceState::Error);
    }

    void play() override {
        sinkChange();
        if(stateMachine->state_impl() != Output::DeviceState::Error)
            stateMachine->play_impl();
    }
    void pause() override {
        sinkChange();
        if(stateMachine->state_impl() != Output::DeviceState::Error) {
            stateMachine->play_impl();
            stateMachine->pause_impl();
        }
    }
    void stop() override {
        sinkChange();
        if(stateMachine->state_impl() != Output::DeviceState::Error)
            assert(stateMachine->state_impl() == Output::DeviceState::Stopped);
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Error;
    }

    void sinkChange() override {
        try {
            stateMachine->changeDevice();
            if(stateMachine->asioOutput) {
                auto ptr = stateMachine->changeState<Stopped>();
                assert(ptr == this);
                assert(stateMachine->state_impl() == Output::DeviceState::Stopped);
            }
        }
        catch(...) {
            ERROR_LOG(stateMachine->logject) << boost::current_exception_diagnostic_information();
        }
    }
};

struct Stop : virtual State {
    explicit Stop(StateChanged& sc) : State(sc) {}
    virtual void stop() override {
        stateMachine->asioOutput->stop();
        auto ptr = stateMachine->changeState<Stopped>();
        assert(ptr == this);
    }
};

struct Tell : virtual State {
    explicit Tell(StateChanged& sc) : State(sc) {}
    virtual chrono::milliseconds tell() const override {
        assert(playman->currentPlaylist());
        auto playlist = playman->currentPlaylist();
        assert(playlist);
        if(!*playlist)
            return 0ms;
        return playlist->currentTrack()->tell();
    }
};

struct Paused;

struct Playing : virtual State, Stop, Tell {
    explicit Playing(StateChanged& sc) : State(sc), Stop(sc), Tell(sc) {
        assert(stateMachine->asioOutput);
        stateChanged(Output::DeviceState::Playing);
    }

    void pause() override {
        stateMachine->asioOutput->pause();
        auto ptr = stateMachine->changeState<Paused>();
        assert(ptr == this);
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Playing;
    }

    void sinkChange() override {
        auto time(tell());
        stateMachine->asioOutput->stop();
        stateMachine->changeDevice();
        stateMachine->play_impl();
        stateMachine->playman.currentPlaylist()->currentTrack()->seek(time);
    }
};

struct Paused : virtual State, Stop, Tell {
    explicit Paused(StateChanged& sc) : State(sc), Stop(sc), Tell(sc) {
        assert(stateMachine->asioOutput);
        stateChanged(Output::DeviceState::Paused);
    }

    void play() override {
        stateMachine->asioOutput->play();
        auto ptr = stateMachine->changeState<Playing>();
        assert(ptr == this);
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Paused;
    }

    void sinkChange() override {
        auto time(tell());
        stateMachine->asioOutput->stop();
        stateMachine->changeDevice();
        stateMachine->play_impl();
        stateMachine->playman.currentPlaylist()->currentTrack()->seek(time);
        stateMachine->pause_impl();
    }
};

Player::impl::impl(Kernel &kernel) :
    playman(kernel.getPlaylistManager()),
    outman(kernel.getOutputManager()),
    confman(kernel.getConfigManager()),
    mu(),
    stateChanged(),
    currentState_((State::stateMachine = this, //init statics before first construction
                   State::playman = &playman, //dirty comma operator usage
                   std::make_shared<Stopped>(stateChanged))),
    end(false),
//    playerThread(&impl::async_loop, this),
    logject(logging::keywords::channel = "StateMachine")
{
    conf.putNode("buffer size", buffer_size);
    playman.getTrackChangedSignal().connect(&impl::trackChangeSlot, this);
    outman.getPlayerSinkChangedSignal().connect(&impl::sinkChangeSlot, this);
    confman.getLoadedSignal().connect(&impl::loadedSlot, this);
}

Player::impl::~impl() {
    stop();
    end = true;
    if(playerThread.joinable()) {
        TRACE_LOG(logject) << "Closing thread";
        playerThread.join();
    }
}

void Player::impl::play_impl() {
    currentState_->play();
}

void Player::impl::pause_impl() {
    currentState_->pause();
}

void Player::impl::stop_impl() {
    currentState_->stop();
    if(playman.currentPlaylist() && *playman.currentPlaylist()) {
        playman.currentPlaylist()->currentTrack()->reset();
        playman.currentPlaylist()->currentTrack()->close();
    }
}

void Player::impl::seek(chrono::milliseconds dur) {
    TRACE_LOG(logject) << "Seek...";
    if(playman.currentPlaylist()) {
        playman.currentPlaylist()->seek(dur);
    }
}

chrono::milliseconds Player::impl::tell_impl() {
    return currentState_->tell();
}

Output::DeviceState Player::impl::state_impl() {
    return currentState_->state();
}

void Player::impl::trackChangeSlot(const Track& track) {
    auto cp = playman.currentPlaylist();
    assert(cp);
    try {
        if(*cp && asioOutput) {
            assert(track == *cp->currentTrack());
            if(asioOutput->currentSpecs() != track.getAudioSpecs()) {
                LOG(logject) << "Changing sink specs for new track";
                asioOutput->stop();
                this_thread::sleep_for(100ms);
                asioOutput->prepare(const_cast<Track&>(track).getAudioSpecs());
                this_thread::sleep_for(100ms);
                asioOutput->play();
            }
        }
    }
    catch(DeviceException& e) {
        ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
        changeState<Error>();
    }
    catch(...) {
        ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
        playman.currentPlaylist()->next();
    }
}

void Player::impl::changeDevice() {
    TRACE_LOG(logject) << "changeDevice()";
    if(asioOutput) {
        asioOutput->cancel();
        asioOutput->stop();
    }
//    lock_guard l(sinkWrapper_.mu);
    asioOutput = std::move(outman.createASIOSink());
}

void Player::impl::sinkChangeSlot() {
    TRACE_LOG(logject) << "sinkChangeSlot()";
    unique_lock l(mu);
    try {
        currentState_->sinkChange();
    }
    catch(...) {
        ERROR_LOG(logject) << boost::current_exception_diagnostic_information();

        stop_impl();
        changeState<Error>();
    }
}

Player::Player(Kernel& kernel) :
    pimpl(new impl(kernel))
{}

Player::~Player() {}

void Player::play() {
    pimpl->play();
}

void Player::pause() {
    pimpl->pause();
}

void Player::stop() {
    pimpl->stop();
}

DeviceState Player::state() const {
    return pimpl->state();
}

void Player::seek(chrono::milliseconds dur) {
    pimpl->seek(dur);
}

chrono::milliseconds Player::tell() const {
    return pimpl->tell();
}

Signals::Player::StateChanged& Player::stateChangedSignal() const {
    return pimpl->stateChanged;
}

} // namespace Core
} // namespace Melosic

