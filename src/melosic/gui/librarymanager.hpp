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

#ifndef MELOSIC_QML_LIBRARY_HPP
#define MELOSIC_QML_LIBRARY_HPP

#include <QObject>

namespace Melosic {

namespace Library {
class Manager;
}

class LibraryManager : public QObject {
    Q_OBJECT

    Library::Manager& libman;

  public:
    explicit LibraryManager(Library::Manager& libman, QObject* parent = nullptr);

    Library::Manager& getLibraryManager() const;

Q_SIGNALS:

  public Q_SLOTS:
};

} // namespace Melosic

#endif // MELOSIC_QML_LIBRARY_HPP
