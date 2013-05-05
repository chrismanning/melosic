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

#include <functional>
#include <deque>
#include <algorithm>

#include <QStringList>
#include <QMimeData>
#include <QPainter>

#include <boost/lexical_cast.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;

#include <melosic/core/track.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/melin/playlist.hpp>

#include "playlistmodel.hpp"

namespace Melosic {

Logger::Logger PlaylistModel::logject(logging::keywords::channel = "PlaylistModel");

PlaylistModel::PlaylistModel(std::shared_ptr<Core::Playlist> playlist, QObject* parent)
    : QAbstractListModel(parent),
      playlist(playlist) {}

int PlaylistModel::rowCount(const QModelIndex& /*parent*/) const {
    return playlist->size();
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
                return data(index, TrackRoles::SourceName);
            case TrackRoles::SourceName:
                return QString::fromStdString(track.sourceName());
//            case TrackRoles::Category:
//                return data(index, TrackRoles::AlbumArtist).toString() + " - "
//                        + data(index, TrackRoles::Album).toString() + " - "
//                        + data(index, TrackRoles::Date).toString();
            case TrackRoles::Title:
                return QString::fromStdString(track.getTag("title"));
            case TrackRoles::Artist:
                return QString::fromStdString(track.getTag("artist"));
            case TrackRoles::Album:
                return QString::fromStdString(track.getTag("album"));
            case TrackRoles::AlbumArtist: {
                std::string aa = track.getTag("albumartist");
                return QString::fromStdString(aa == "?" ? track.getTag("artist") : aa);
            }
            case TrackRoles::TrackNumber:
                return QString::fromStdString(track.getTag("tracknumber"));
            case TrackRoles::Genre:
                return QString::fromStdString(track.getTag("genre"));
            case TrackRoles::Date:
                return QString::fromStdString(track.getTag("date"));
            case TrackRoles::Duration: {
                return chrono::duration_cast<chrono::seconds>(track.duration()).count();
            }
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
            auto text = data(index, (int)TrackRoles::SourceName).toString();
            stream << text;
        }
    }

    mimeData->setData("application/vnd.text.list", encodedData);
    return mimeData;
}

QHash<int, QByteArray> PlaylistModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[TrackRoles::SourceName] = "source";
    roles[TrackRoles::Title] = "title";
    roles[TrackRoles::Artist] = "artist";
    roles[TrackRoles::Album] = "album";
    roles[TrackRoles::TrackNumber] = "tracknumber";
    roles[TrackRoles::Genre] = "genre";
    roles[TrackRoles::Date] = "year";
    roles[TrackRoles::Duration] = "duration";
    return roles.unite(QAbstractListModel::roleNames());
}

bool PlaylistModel::insertTracks(int row, QList<QUrl> filenames) {
    LOG(logject) << "In insertTracks(int, QList<QUrl>)";
    boost::range::sort(filenames);
    auto fun([] (QUrl url) {
        return url.toLocalFile().toStdString();
    });
    TRACE_LOG(logject) << "Inserting " << filenames.count() << " files";
    for(auto&& v : filenames | transformed(fun))
        TRACE_LOG(logject) << v;

    std::deque<boost::filesystem::path> tmp;
    boost::range::push_back(tmp, filenames | reversed | transformed(fun));
    return insertTracks(row, tmp);
}

bool PlaylistModel::insertTracks(int row, ForwardRange<const boost::filesystem::path> filenames) {
    TRACE_LOG(logject) << "In insertTracks(ForwardRange<path>)";
    auto beg = ++row > playlist->size() ? playlist->size() : row;
    beginInsertRows(QModelIndex(), beg, beg + boost::distance(filenames) -1);

    int r = true;
    try {
        playlist->emplace(std::next(playlist->begin(), beg), filenames);
    }
    catch(Exception& e) {
        ERROR_LOG(logject) << boost::diagnostic_information(e);
        //TODO: error handling
        r = false;
    }

    endInsertRows();
    return r;
}

//QStringList PlaylistModel::appendFiles(const ForwardRange<const boost::filesystem::path>& filenames) {
//    QStringList failList;
//    if(filenames.empty())
//        return failList;
//    auto beg = playlist->size();
//    beginInsertRows(QModelIndex(), beg, beg + boost::distance(filenames) -1);
//    for(const auto& filename : filenames) {
//        try {
//            TRACE_LOG(logject) << "in appendFiles: " << filename;
//            playlist->emplace_back(filename);
//        }
//        catch(FileException& e) {
//            if(auto* path = boost::get_error_info<ErrorTag::FilePath>(e)) {
//                std::string tmp(path->string());
//                ERROR_LOG(logject) << (tmp += ": file error");
//                failList << QString::fromStdString(tmp);
//            }
//        }
//        catch(FileNotFoundException& e) {
//            if(auto* path = boost::get_error_info<ErrorTag::FilePath>(e)) {
//                std::string tmp(path->string());
//                ERROR_LOG(logject) << (tmp += ": file not found");
//                failList << QString::fromStdString(tmp);
//            }
//        }
//        catch(UnsupportedFileTypeException& e) {
//            if(auto* path = boost::get_error_info<ErrorTag::FilePath>(e)) {
//                std::string tmp(path->string());
//                ERROR_LOG(logject) << (tmp += ": file type unsupported");
//                failList << QString::fromStdString(tmp);
//            }
//        }
//        catch(DecoderException& e) {
//            if(auto* path = boost::get_error_info<ErrorTag::FilePath>(e)) {
//                failList << QString::fromStdString(path->string()) + ": file could not be decoded";
//            }
//        }
//    }
//    endInsertRows();
//    LOG(logject) << "Inserted " << playlist->size() - beg << " files as tracks.";
//    return failList;
//}

//QStringList PlaylistModel::appendFiles(QList<QUrl> filenames) {
//    TRACE_LOG(logject) << "QML filenames count: " << filenames.size();
//    std::forward_list<boost::filesystem::path> paths;
//    for(QUrl v : filenames) {
//        assert(v.isLocalFile());
//        paths.emplace_after(paths.before_begin(), v.toLocalFile().toStdString());
//    }

//    return appendFiles(paths);
//}

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

//    for(const auto& filename : filenames) {
//        begin = emplace(begin, kernel.getDecoderManager(), filename.toStdString());
//    }
    return false;

    return true;
}

bool PlaylistModel::moveRows(const QModelIndex&, int sourceRow, int count,
                             const QModelIndex&, int destinationChild)
{
    if(sourceRow == destinationChild)
        return false;
    if(destinationChild > rowCount())
        destinationChild = rowCount();
    auto beg = playlist->begin();
    auto dest = std::next(beg, destinationChild);
    const auto s = playlist->size();
    if(!beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), destinationChild))
        return false;
    std::vector<Core::Playlist::value_type> tmp(std::make_move_iterator(std::next(beg, sourceRow)),
                                                std::make_move_iterator(std::next(beg, sourceRow + count)));
    playlist->erase(sourceRow, sourceRow + count);

    std::move(tmp.begin(), tmp.end(), std::inserter(*playlist, dest));

    endMoveRows();
    assert(playlist->size() == s);

    return playlist->size() == s;
}

bool PlaylistModel::moveRows(int sourceRow, int count, int destinationChild) {
    return moveRows(QModelIndex(), sourceRow, count, QModelIndex(), destinationChild);
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex&) {
    if(row < 0 && (count + row) > rowCount())
        return false;

    const auto s = playlist->size();
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    playlist->erase(row, row + count);
    endRemoveRows();
    assert(playlist->size() == s - count);

    return playlist->size() == s - count;
}

} // namespace Melosic
