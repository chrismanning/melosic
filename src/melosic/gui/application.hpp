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

#ifndef MELOSIC_APPLICATION_HPP
#define MELOSIC_APPLICATION_HPP

#include <QApplication>
#include <QMessageBox>

class Application : public QApplication {
public:
    Application(int argc, char* argv[]) : QApplication(argc, argv) {}

    bool notify(QObject * receiver, QEvent * event) {
        try {
            return QApplication::notify(receiver, event);
        }
        catch(std::exception &e) {
            QMessageBox error;
            error.setText(QString(e.what()));
            return error.exec();
        }
        catch(...) {
            qFatal("Error <unknown> sending event %s to object %s (%s)",
                typeid(*event).name(), qPrintable(receiver->objectName()),
                typeid(*receiver).name());
        }

        // qFatal aborts, so this isn't really necessary
        // but you might continue if you use a different logging lib
        return false;
    }
};

#endif // MELOSIC_APPLICATION_HPP
