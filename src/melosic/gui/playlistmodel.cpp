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

#include <QStringList>
#include <QMimeData>
#include <QPainter>
#include <kcategorizedsortfilterproxymodel.h>
#include <kcategorydrawer.h>
#include <kcategorizedview.h>

#include <boost/lexical_cast.hpp>

#include <melosic/core/track.hpp>
#include <melosic/common/file.hpp>

#include "playlistmodel.hpp"

PlaylistModel::PlaylistModel(Melosic::Kernel& kernel,
                             std::shared_ptr<Melosic::Playlist> playlist,
                             QObject* parent) :
    QAbstractListModel(parent),
    playlist(playlist),
    kernel(kernel),
    logject(logging::keywords::channel = "PlaylistModel") {}

int PlaylistModel::rowCount(const QModelIndex& /*parent*/) const {
    return playlist->size();
}

bool PlaylistModel::insertRows(int /*row*/, int /*count*/, const QModelIndex& /*parent*/) {
    TRACE_LOG(logject) << "Inserting rows...";
    return false;
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid())
        return QVariant();

    assert(playlist);
    if(index.row() >= playlist->size())
        return QVariant();

    try {
        const auto& track = (*playlist)[index.row()];
        switch(role) {
            case Qt::DisplayRole:
                return data(index, Melosic::TrackRoles::SourceName);
            case Melosic::TrackRoles::SourceName:
                return QString::fromStdString(track.sourceName());
            case Melosic::TrackRoles::Category:
            case KCategorizedSortFilterProxyModel::CategoryDisplayRole:
            case KCategorizedSortFilterProxyModel::CategorySortRole:
                return data(index, Melosic::TrackRoles::AlbumArtist).toString() + " - "
                        + data(index, Melosic::TrackRoles::Album).toString() + " - "
                        + data(index, Melosic::TrackRoles::Date).toString();
            case Melosic::TrackRoles::Title:
                return QString::fromStdString(track.getTag("title"));
            case Melosic::TrackRoles::Artist:
                return QString::fromStdString(track.getTag("artist"));
            case Melosic::TrackRoles::Album:
                return QString::fromStdString(track.getTag("album"));
            case Melosic::TrackRoles::AlbumArtist: {
                std::string aa("albumartist");
                std::string a = track.getTag(aa);
                return QString::fromStdString(a == "?" ? track.getTag("artist") : a);
            }
            case Melosic::TrackRoles::TrackNumber:
                return QString::fromStdString(track.getTag("tracknumber"));
            case Melosic::TrackRoles::Genre:
                return QString::fromStdString(track.getTag("genre"));
            case Melosic::TrackRoles::Date:
                return QString::fromStdString(track.getTag("date"));
            case Melosic::TrackRoles::Length:
                return QString::fromStdString(track.getTag("length"));
            default:
                return QVariant();
        }
    }
    catch(...) {
        return QVariant();
    }
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
            auto text = data(index, (int)Melosic::TrackRoles::SourceName).toString();
            stream << text;
        }
    }

    mimeData->setData("application/vnd.text.list", encodedData);
    return mimeData;
}

QHash<int, QByteArray> PlaylistModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[Melosic::TrackRoles::SourceName] = "source";
    roles[Melosic::TrackRoles::Category] = "category";
    roles[Melosic::TrackRoles::Title] = "title";
    roles[Melosic::TrackRoles::Artist] = "artist";
    roles[Melosic::TrackRoles::Album] = "album";
    roles[Melosic::TrackRoles::TrackNumber] = "tracknumber";
    roles[Melosic::TrackRoles::Genre] = "genre";
    roles[Melosic::TrackRoles::Date] = "year";
    roles[Melosic::TrackRoles::Length] = "length";
    return roles.unite(QAbstractListModel::roleNames());
}

void PlaylistModel::appendTracks(const Melosic::ForwardRange<Melosic::Track>& tracks) {
    auto beg = playlist->size();
    beginInsertRows(QModelIndex(), beg, beg + boost::distance(tracks) -1);
    playlist->insert(playlist->end(), tracks);
    endInsertRows();
    TRACE_LOG(logject) << "Inserted " << boost::distance(tracks) << " tracks.";
}

QStringList PlaylistModel::appendFiles(const Melosic::ForwardRange<const boost::filesystem::path>& filenames) {
    QStringList failList;
    if(filenames.empty())
        return failList;
    auto beg = playlist->size();
    beginInsertRows(QModelIndex(), beg, beg + boost::distance(filenames) -1);
    for(const auto& filename : filenames) {
        try {
            playlist->push_back({kernel.getDecoderManager(), filename});
        }
        catch(Melosic::UnsupportedFileTypeException& e) {
            if(auto* path = boost::get_error_info<Melosic::ErrorTag::FilePath>(e))
                failList << QString::fromStdString(path->string()) + ": file type not supported";
        }
        catch(Melosic::DecoderException& e) {
            if(auto* path = boost::get_error_info<Melosic::ErrorTag::FilePath>(e))
                failList << QString::fromStdString(path->string()) + ": file could not be decoded";
        }
    }
    endInsertRows();
    LOG(logject) << "Inserted " << playlist->size() - beg << " files as tracks.";
    return failList;
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

CategoryDrawer::CategoryDrawer(KCategorizedView* view) : KCategoryDrawerV3(view) {}

void CategoryDrawer::drawCategory(const QModelIndex& index,
                                  int /*sortRole*/,
                                  const QStyleOption& option,
                                  QPainter* painter) const
{
    KCategorizedView* sm = view();

    painter->drawText(option.rect, sm->model()->data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole).toString());
}
