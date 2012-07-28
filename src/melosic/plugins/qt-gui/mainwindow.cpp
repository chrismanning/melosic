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
#include <QFileDialog>

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

void MainWindow::on_actionOpen_triggered()
{
    auto file = QFileDialog::getOpenFileName(this, tr("Open file"), ".", tr("Audio Files (*.flac)"));
    qDebug() << file;
    filename = file;
}

void MainWindow::on_actionPlay_triggered()
{
    if(filename.length() > 0) {
        //TODO: hook in to Qt event loop
        auto input = kernel->getInputManager()->openFile(filename.toStdString().c_str());
        auto output = kernel->getOutputManager()->getDefaultOutput();
        output->prepareDevice(input->getAudioSpecs());
        output->render(input->getDecodeRange());
    }
    else {
        qDebug("Nothing to play");
    }
}
