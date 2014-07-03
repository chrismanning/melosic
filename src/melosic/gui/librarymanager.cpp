/**************************************************************************
**  Copyright (C) 2014 Christian Manning
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

#include <melosic/melin/library.hpp>
#include <melosic/common/signal.hpp>

#include "librarymanager.hpp"

namespace Melosic {

LibraryManager::LibraryManager() {
    connect(this, &LibraryManager::libraryManagerChanged, [this]() {
        assert(m_libman);
        if(!m_libman)
            return;
        m_libman->getScanStartedSignal().connect([this]() {
            std::clog << "LibraryManager: scan started\n";
            Q_EMIT scanStarted();
        });
        m_libman->getScanEndedSignal().connect([this]() {
            std::clog << "LibraryManager: scan ended\n";
            Q_EMIT scanEnded();
        });
    });
}

LibraryManager* LibraryManager::instance() {
    static LibraryManager libman;
    return &libman;
}

const std::shared_ptr<Library::Manager>& LibraryManager::getLibraryManager() const { return m_libman; }

void LibraryManager::setLibraryManager(const std::shared_ptr<Library::Manager>& libman) {
    m_libman = libman;
    assert(m_libman);
    Q_EMIT libraryManagerChanged();
}

} // namespace Melosic
