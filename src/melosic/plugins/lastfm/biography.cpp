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

#include "biography.hpp"
#include "service.hpp"
#include "track.hpp"

namespace LastFM {

//BiographyWidget::BiographyWidget(std::weak_ptr<Service> lastserv, QWidget *parent)
//    : QWidget(parent),
//      lastserv(lastserv)
//{
//    text = new QLabel;
//    layout = new QVBoxLayout;
//    layout->addWidget(text);
//    this->setLayout(layout);
//    refresh();
//}

//void BiographyWidget::refresh() {
//    std::shared_ptr<Service> ptr(lastserv.lock());
//    if(ptr && ptr->currentTrack()) {
//        Melosic::Thread::Manager* tman = ptr->getThreadManager();
//        tman->enqueue([this](std::shared_ptr<Service> ptr) {
//            if(ptr->currentTrack()->fetchInfo().get()) {
//                text->setText(ptr->currentTrack()->getUrl().string().c_str());
//                update();
//            }
//        }, ptr);
//    }
//}

}//namespace LastFM
