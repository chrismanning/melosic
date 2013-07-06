/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#include <QSharedPointer>

#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;

#include <melosic/core/playlist.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/gui/playlistmodel.hpp>

#include "playlistmanagermodel.hpp"

namespace Melosic {

PlaylistManagerModel::PlaylistManagerModel(Playlist::Manager& playman, QObject* parent)
    : QAbstractListModel(parent),
      playman(playman),
      logject(logging::keywords::channel = "PlaylistManagerModel")
{}

Qt::ItemFlags PlaylistManagerModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

    if(index.isValid())
        return Qt::ItemIsEditable | defaultFlags;
    return defaultFlags;
}

QVariant PlaylistManagerModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid() || index.row() >= playman.count())
        return QVariant();

    Q_ASSERT(playman.range()[index.row()] != nullptr);

    switch(role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return QString::fromStdString(playman.range()[index.row()]->getName());
        case PlaylistModelRole: {
            auto ptr = playman.range()[index.row()];
            if(playlists.contains(ptr.get()))
                return QVariant::fromValue(playlists[ptr.get()]);
            auto nthis = const_cast<PlaylistManagerModel*>(this);

            return QVariant::fromValue(nthis->playlists[ptr.get()] = new PlaylistModel(ptr, nthis));
//            auto it = playlists.find(QString::fromStdString(playman.range()[index.row()]->getName()));
//            return *it ? QVariant::fromValue(*it) : QVariant();
        }
        default:
            return QVariant();
    }
}

QVariant PlaylistManagerModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const {
    return tr("Playlists");
}

QHash<int, QByteArray> PlaylistManagerModel::roleNames() const {
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles[PlaylistModelRole] = "playlistModel";
    return roles;
}

int PlaylistManagerModel::rowCount(const QModelIndex& /*parent*/) const {
    return playman.count();
}

bool PlaylistManagerModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if(!index.isValid() || index.row() >= playman.count() || !(role & Qt::EditRole) || !value.isValid())
        return false;

    playman.range()[index.row()]->setName(value.toString().toStdString());
    return true;
}

bool PlaylistManagerModel::insertRows(int row, int count, const QModelIndex&) {
    if(row > playman.count())
        return false;

    TRACE_LOG(logject) << "row: " << row << "; count: " << count;
    beginInsertRows(QModelIndex(), row, row+count-1);
//    for(int i = row; i < row+count; i++) {
//        auto it = playman.insert(std::next(playman.range().begin(), row), count);
//        if(it != playman.range().end()) {
//            if(auto ptr = playman.range()[i]) {
//                playlists[QString::fromStdString(ptr->getName())] = new PlaylistModel(ptr);
//            }
//        }
//        else {
//            ERROR_LOG(logject) << "Failed to add playlist";
//            return false;
//        }
//        TRACE_LOG(logject) << "Playlists: " << playman.count();
//    }
    playman.insert(std::next(playman.range().begin(), row), count);
    endInsertRows();
    return true;
}

bool PlaylistManagerModel::removeRows(int row, int count, const QModelIndex&) {
    playman.erase(playman.range() | sliced(row, count));
    return true;
}

QModelIndex PlaylistManagerModel::index(int row, int column, const QModelIndex& parent) const {
    return QAbstractListModel::index(row, column, parent);
}

} // namespace Melosic
