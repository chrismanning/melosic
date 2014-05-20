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

#include <asio/io_service.hpp>
#include <asio/signal_set.hpp>

#include <melosic/melin/kernel.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/melin/plugin.hpp>
#include <melosic/melin/input.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/melin/encoder.hpp>
#include <melosic/common/thread.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/common/directories.hpp>
#include <melosic/core/track.hpp>
#include <melosic/melin/library.hpp>
#include <melosic/melin/logging.hpp>

namespace Melosic {
namespace Core {

struct Kernel::impl {
    impl() :
        confman("melosic.conf"),
        plugman(confman),
        io_service(),
        signal_set(io_service),
        outman(confman, io_service),
        null_worker(new asio::io_service::work(io_service)),
        tman(&io_service),
        null_worker_(std::move(null_worker)),
        inman(),
        decman(inman, tman),
        encman(),
        libman(confman, decman, plugman, tman),
        playlistman()
    {
        signal_set.add(SIGABRT);
        signal_set.add(SIGINT);
        signal_set.add(SIGTERM);
        signal_set.add(SIGQUIT);
        signal_set.async_wait([this](asio::error_code ec, int signo) {
            if(ec)
                return;
            WARN_LOG(logject) << "SIGNAL RAISED: " << signo;
            std::quick_exit(signo);
        });
    }

    ~impl() {
        signal_set.cancel();
    }

    Config::Manager confman;
    Plugin::Manager plugman;
    asio::io_service io_service;
    asio::signal_set signal_set;
    Output::Manager outman;
    std::unique_ptr<asio::io_service::work> null_worker;
    Thread::Manager tman;
    std::unique_ptr<asio::io_service::work> null_worker_;
    Input::Manager inman;
    Decoder::Manager decman;
    Encoder::Manager encman;
    Library::Manager libman;
    Melosic::Playlist::Manager playlistman;
    Logger::Logger logject{logging::keywords::channel = "Kernel"};
};

Kernel::Kernel() : pimpl(new impl) {}

Kernel::~Kernel() {
    try {
        getConfigManager().saveConfig();
    }
    catch(...) {
        std::clog << boost::current_exception_diagnostic_information() << std::endl;
    }
}

Config::Manager& Kernel::getConfigManager() {
    return pimpl->confman;
}

Input::Manager& Kernel::getInputManager() {
    return pimpl->inman;
}

Decoder::Manager& Kernel::getDecoderManager() {
    return pimpl->decman;
}

Output::Manager& Kernel::getOutputManager() {
    return pimpl->outman;
}

Encoder::Manager& Kernel::getEncoderManager() {
    return pimpl->encman;
}

Plugin::Manager& Kernel::getPluginManager() {
    return pimpl->plugman;
}

Thread::Manager& Kernel::getThreadManager() {
    return pimpl->tman;
}

Melosic::Playlist::Manager& Kernel::getPlaylistManager() {
    return pimpl->playlistman;
}

Library::Manager& Kernel::getLibraryManager() {
    return pimpl->libman;
}

asio::io_service& Kernel::getIOService() {
    return pimpl->io_service;
}

} // namespace Core
} // namespace Melosic

