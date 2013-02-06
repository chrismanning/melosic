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

#include "playlistmodel.hpp"
#include <QStringList>
#include <QMimeData>

#include <melosic/core/track.hpp>
#include <melosic/common/file.hpp>

PlaylistModel::PlaylistModel(Melosic::Kernel& kernel,
                             std::shared_ptr<Melosic::Playlist> playlist,
                             QObject* parent) :
    QAbstractListModel(parent),
    playlist(playlist),
    kernel(kernel),
    logject(boost::log::keywords::channel = "PlaylistModel") {}

int PlaylistModel::rowCount(const QModelIndex& /*parent*/) const {
    return playlist->size();
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid())
        return QVariant();

    if(index.row() >= playlist->size())
        return QVariant();

    if(role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto& track = (*playlist)[index.row()];
        return QString::fromStdString(track.sourceName() + " " + track.getTag("artist"));
    }
    else if(role == (int)Melosic::DataRoles::SourceName) {
        const auto& track = (*playlist)[index.row()];
        return QString::fromStdString(track.sourceName() + " " + track.getTag("artist"));
    }
    else
        return QVariant();
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

    if(index.isValid())
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}

QStringList PlaylistModel::mimeTypes() const {
    QStringList types;
    types << "application/vnd.text.list";
    return types;
}

QMimeData* PlaylistModel::mimeData(const QModelIndexList& indexes) const {
    QMimeData* mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for(const QModelIndex& index : indexes) {
        if(index.isValid()) {
            auto text = data(index, (int)Melosic::DataRoles::SourceName).toString();
            stream << text;
        }
    }

    mimeData->setData("application/vnd.text.list", encodedData);
    return mimeData;
}

bool PlaylistModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                 int row, int column, const QModelIndex& parent)
{
    if(action == Qt::IgnoreAction)
        return true;

    for(const auto& format : data->formats()) {
        TRACE_LOG(logject) << format.toStdString();
    }

    if(!data->hasFormat("application/vnd.text.list"))
        return false;

    if(column > 0)
        return false;

    auto begin = playlist->begin();

    if(row != -1)
        begin += row;
    else if(parent.isValid())
        begin += parent.row();
    else
        begin += rowCount(QModelIndex());

    QByteArray encodedData = data->data("application/vnd.text.list");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QStringList filenames;
    int rows = 0;

    while(!stream.atEnd()) {
        QString text;
        stream >> text;
        filenames << text;
        ++rows;
    }

    for(const auto& filename : filenames) {
        begin = emplace(begin, kernel.getDecoderManager(), filename.toStdString());
    }

    return true;
}
