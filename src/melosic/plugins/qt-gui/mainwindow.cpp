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

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(IKernel * k, QWidget * parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), kernel(k)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    qDebug("Destroying main window");
    delete ui;
}

extern "C" int startEventLoop(int argc, char ** argv, IKernel * k) {
    QApplication app(argc, argv);
    k->loadAllPlugins();
    MainWindow win(k);
    win.show();
    return app.exec();
}
