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

#include <csignal>

#include <asio/io_service.hpp>

#include <melosic/melin/kernel.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/melin/plugin.hpp>
#include <melosic/melin/input.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/melin/encoder.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/common/directories.hpp>
#include <melosic/core/track.hpp>
#include <melosic/melin/library.hpp>
#include <melosic/melin/logging.hpp>

namespace Melosic {
namespace Core {

static void signal_handler(int signo) {
    std::quick_exit(signo);
}

struct Kernel::impl {
    impl()
        : confman(new Config::Manager{"melosic.conf"}),
          plugman(new Plugin::Manager{confman}),
          io_service(),
          outman(new Output::Manager{confman, io_service}),
          null_worker(new asio::io_service::work(io_service)),
          inman(new Input::Manager{}),
          decman(new Decoder::Manager{inman, plugman}),
          encman(new Encoder::Manager{}),
          libman(new Library::Manager{confman, decman, plugman}),
          playlistman(new Melosic::Playlist::Manager{})
    {
        std::signal(SIGABRT, signal_handler);
        std::signal(SIGINT, signal_handler);
        std::signal(SIGQUIT, signal_handler);
        std::signal(SIGTERM, signal_handler);
    }

    ~impl() {}

    std::shared_ptr<Config::Manager> confman;
    std::shared_ptr<Plugin::Manager> plugman;
    asio::io_service io_service;
    std::shared_ptr<Output::Manager> outman;
    std::unique_ptr<asio::io_service::work> null_worker;
    std::shared_ptr<Input::Manager> inman;
    std::shared_ptr<Decoder::Manager> decman;
    std::shared_ptr<Encoder::Manager> encman;
    std::shared_ptr<Library::Manager> libman;
    std::shared_ptr<Melosic::Playlist::Manager> playlistman;
    Logger::Logger logject{logging::keywords::channel = "Kernel"};
};

Kernel::Kernel() : pimpl(new impl) {}

Kernel::~Kernel() {
    try {
        pimpl->confman->saveConfig();
    } catch(...) {
        std::clog << boost::current_exception_diagnostic_information() << std::endl;
    }
}

std::shared_ptr<Config::Manager> Kernel::getConfigManager() { return pimpl->confman; }

std::shared_ptr<Input::Manager> Kernel::getInputManager() { return pimpl->inman; }

std::shared_ptr<Decoder::Manager> Kernel::getDecoderManager() { return pimpl->decman; }

std::shared_ptr<Output::Manager> Kernel::getOutputManager() { return pimpl->outman; }

std::shared_ptr<Encoder::Manager> Kernel::getEncoderManager() { return pimpl->encman; }

std::shared_ptr<Plugin::Manager> Kernel::getPluginManager() { return pimpl->plugman; }

std::shared_ptr<Melosic::Playlist::Manager> Kernel::getPlaylistManager() { return pimpl->playlistman; }

std::shared_ptr<Library::Manager> Kernel::getLibraryManager() { return pimpl->libman; }

asio::io_service& Kernel::getIOService() { return pimpl->io_service; }

} // namespace Core
} // namespace Melosic
