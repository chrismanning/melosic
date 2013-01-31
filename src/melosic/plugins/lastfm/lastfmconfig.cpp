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

#include "lastfmconfig.hpp"

#include <QWidget>
#include <QLabel>
#include <QProgressDialog>
#include <QDesktopServices>
#include <QMessageBox>

#include <melosic/common/string.hpp>
#include <melosic/gui/configwidget.hpp>
#include <melosic/core/logging.hpp>

namespace Melosic {

static Logger::Logger logject(boost::log::keywords::channel = "LastFMConfig");

LastFmConfig::LastFmConfig() :
    Configuration("LastFM")
{}

LastFmConfig::~LastFmConfig() {}

LastFmConfigWidget::LastFmConfigWidget(Configuration& conf, QWidget* parent)
    : GenericConfigWidget(conf, parent)
{
    authButton = new QPushButton("Authenticate");
    authButton->setToolTip("Only do this once per user!!\nIf \"Session Key\" is already present do NOT do this");
    connect(authButton, &QPushButton::clicked, this, &LastFmConfigWidget::authenticate);
    layout->addWidget(authButton);
}

void LastFmConfigWidget::apply() {
    GenericConfigWidget::apply();

    if(boost::get<std::string>(conf.getNode("session key")) == "" &&
            boost::get<std::string>(conf.getNode("username")) != "")
    {
        authenticate();
    }
}

void LastFmConfigWidget::authenticate() {
//    QMap<QString, QString> params;
//    params["method"] = "auth.getToken";
//    QProgressDialog progress;
//    progress.setMinimum(0);
//    progress.setMaximum(0);
//    progress.setLabelText("Getting session token...");
//    TRACE_LOG(logject) << "Getting session token...";
//    QNetworkReply* reply = lastfm::ws::post(params, false);
////    progress.connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(cancel()));
//    progress.connect(reply, &QNetworkReply::finished, [&progress] () {progress.close();});
//    progress.exec();

////    if(progress.wasCanceled()) {
////        TRACE_LOG(logject) << "Canceled";
////        reply->abort();
////        return;
////    }

//    QString token;
//    {
//        lastfm::XmlQuery response;
//        if(!response.parse(reply->readAll())) {
//            TRACE_LOG(logject) << "XML response parse fail: " << reply->readAll().constData();
//            return;
//        }
//        token = response["token"].text();
//    }

//    TRACE_LOG(logject) << "Got token: " << token.toStdString();

//    QString lurl = QString("http://www.last.fm/api/auth/?api_key=")
//            + lastfm::ws::ApiKey
//            + "&token="
//            + token;
//    TRACE_LOG(logject) << "Opening URL: " << lurl.toStdString();
//    QDesktopServices::openUrl(lurl);

//    if(QMessageBox::question(this, "Continue?", "If web authentication was successful, please continue.",
//                             QMessageBox::Yes | QMessageBox::No,
//                             QMessageBox::Yes) != QMessageBox::Yes) {
//        return;
//    }
//    TRACE_LOG(logject) << "Web authenticated";

//    params["method"] = "auth.getSession";
//    params["token"] = token;

//    reply = lastfm::ws::post(params, false);
////    progress.connect(reply, &QNetworkReply::error, [&progress] (QNetworkReply::NetworkError&) -> void {progress.cancel();});
//    progress.connect(reply, &QNetworkReply::finished, [&progress] () {progress.close();});
//    progress.exec();

//    {
//        lastfm::XmlQuery response;
//        if(!response.parse(reply->readAll()))
//            return;
//        conf.putNode("session key", response["session"]["key"].text().toStdString());
//    }
}

ConfigWidget* LastFmConfig::createWidget() {
    return new LastFmConfigWidget(*this);
}

QIcon* LastFmConfig::getIcon() const {
    return nullptr;
}

LastFmConfig* LastFmConfig::clone() const {
    return new LastFmConfig(*this);
}

} //end namespace Melosic
