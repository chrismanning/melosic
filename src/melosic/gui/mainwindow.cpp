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
#include <boost/iostreams/copy.hpp>
namespace io = boost::iostreams;

#include <melosic/core/wav.hpp>

MainWindow::MainWindow(QWidget * parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    qDebug("Destroying main window");
    delete ui;
}

void MainWindow::loadPlugins() {
    kernel.loadPlugin("flac.melin");
}

void MainWindow::on_actionOpen_triggered()
{
    auto file = QFileDialog::getOpenFileName(this, tr("Open file"), ".", tr("Audio Files (*.flac)"));
    filename = file;
//    files << IO::File(filename.toStdString());
}

void MainWindow::on_actionPlay_triggered()
{
    if(files.length() > 0) {
        //TODO: hook in to Qt event loop
//        auto input_ptr = kernel.getInputManager().openFile(files.first());
//        auto& input = *input_ptr;
//        auto output = Output::WaveFile("test1.wav", input.getAudioSpecs());

////        io::copy(boost::ref(input), output);

//        std::vector<char> buf(4096);
//        std::streamsize total = 0;

//        while((bool)input) {
//            std::streamsize amt;
//            amt = io::read(input, buf.data(), 4096);
//            if(amt > 0) {
//                io::write(output, buf.data(), amt);
//                total += amt;
//            }
//        }
    }
    else {
        qDebug("Nothing to play");
    }
}
