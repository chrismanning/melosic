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
#include <algorithm>

#include <QStringList>
#include <QMimeData>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQmlEngine>
#include <QDebug>

#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;

#include <melosic/core/track.hpp>
#include <melosic/melin/kernel.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/common/signal_core.hpp>
#include <melosic/common/thread.hpp>
#include <melosic/common/optional.hpp>

#include <taglib/tpropertymap.h>

#include "playlistmodel.hpp"

namespace Melosic {

Logger::Logger PlaylistModel::logject(logging::keywords::channel = "PlaylistModel");

PlaylistModel::PlaylistModel(Core::Playlist playlist, Thread::Manager& tman, QObject* parent)
    : QAbstractListModel(parent),
      playlist(playlist),
      tman(tman)
{
    this->playlist.getMutlipleTagsChangedSignal().connect([this] (int start, int end) {
        Q_EMIT dataChanged(index(start), index(end-1));
    });

    connect(this, &PlaylistModel::dataChanged, [this] (const QModelIndex&, const QModelIndex&, const QVector<int>&) {
        m_duration = chrono::duration_cast<chrono::seconds>(this->playlist.duration()).count();
        Q_EMIT durationChanged(m_duration);
    });
}

QString PlaylistModel::name() const {
    return QString::fromStdString(playlist.getName());
}

void PlaylistModel::setName(QString name) {
    playlist.setName(name.toStdString());
    Q_EMIT nameChanged(name);
}

int PlaylistModel::rowCount(const QModelIndex& /*parent*/) const {
    return playlist.size();
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid())
        return {};

    if(index.row() >= playlist.size())
        return {};

    try {
        auto track = playlist.getTrack(index.row());
        if(!track)
            return {};
        switch(role) {
            case Qt::DisplayRole:
                return data(index, TrackRoles::FileName);
            case TrackRoles::FileName:
                return QString::fromStdString(track->filePath().filename().string());
            case TrackRoles::FilePath:
                return QString::fromStdString(track->filePath().string());
            case TrackRoles::FileExtension:
                return QString::fromStdString(track->filePath().extension().string());
            case TrackRoles::TagsReadable:
                return QVariant::fromValue(track->tagsReadable());
            case TrackRoles::Duration: {
                return QVariant::fromValue(chrono::duration_cast<chrono::seconds>(track->duration()).count());
            }
            default:
                return {};
        }
    }
    catch(...) {
        return {};
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
            auto text = data(index, (int)TrackRoles::FilePath).toString();
            stream << text;
        }
    }

    mimeData->setData("application/vnd.text.list", encodedData);
    return mimeData;
}

QHash<int, QByteArray> PlaylistModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[TrackRoles::FileName] = "file";
    roles[TrackRoles::FilePath] = "filepath";
    roles[TrackRoles::FileExtension] = "extension";
    roles[TrackRoles::TagsReadable] = "tags_readable";
    roles[TrackRoles::Current] = "is_current";
    roles[TrackRoles::Duration] = "duration";
    return roles.unite(QAbstractListModel::roleNames());
}

bool PlaylistModel::insertTracks(int row, QList<QUrl> filenames) {
    LOG(logject) << "In insertTracks(int, QList<QUrl>)";
    qSort(filenames);
    auto fun = [] (QUrl url) {
        return url.toLocalFile().toStdString();
    };
    TRACE_LOG(logject) << "Inserting " << filenames.count() << " files";

    auto range = filenames | transformed(fun);
    std::vector<boost::filesystem::path> tmp{range.begin(), range.end()};
    Q_ASSERT(static_cast<int>(tmp.size()) == filenames.size());
#ifndef MELOSIC_DISABLE_LOGGING
    for(auto&& v : tmp)
        TRACE_LOG(logject) << v;
#endif
    return insertTracks(row, tmp);
}

struct Refresher {
    Refresher(Core::Playlist p, Thread::Manager& tman, int start, int end, int stride = 1) noexcept
        :
          p(p),
          tman(tman),
          start(start),
          end(end),
          stride(stride)
    {
        assert(stride > 0);
    }

    void operator()() {
        while(start+stride > end)
            --stride;
        p.refreshTracks(start, start+stride);
        start += stride;
        if(start < end)
            tman.enqueue(Refresher(p, tman, start, end, stride));
    }

    Core::Playlist p;
    Thread::Manager& tman;
    int start;
    int end;
    int stride;
};

bool PlaylistModel::insertTracks(int row, ForwardRange<const boost::filesystem::path> filenames) {
    TRACE_LOG(logject) << "In insertTracks(ForwardRange<path>)";

    row = ++row > playlist.size() ? playlist.size() : row;

    beginInsertRows(QModelIndex(), row, row + boost::distance(filenames) -1);

    auto n = playlist.emplace(row, filenames);

    TRACE_LOG(logject) << n << " tracks inserted";

    endInsertRows();

    tman.enqueue(Refresher(playlist, tman, row, row+n, 1));

    return n == boost::distance(filenames);
}

void PlaylistModel::refreshTags(int start, int end) {
    Q_ASSERT(start >= 0);
    TRACE_LOG(logject) << "refreshing tags of tracks " << start << " - " << end;
    end = end < start ? playlist.size() : end+1;
    for(auto& t : playlist.getTracks(start, end)) {
        try {
//            t.reOpen();
//            t.close();
        }
        catch(...) {}
    }
    Q_EMIT dataChanged(index(start), index(end-1));
}

bool PlaylistModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                 int row, int column, const QModelIndex& parent)
{
    assert(false);
    if(action == Qt::IgnoreAction)
        return true;

    for(const auto& format : data->formats()) {
        TRACE_LOG(logject) << format.toStdString();
    }

    if(!data->hasFormat("application/vnd.text.list"))
        return false;

    if(column > 0)
        return false;

    QByteArray encodedData = data->data("application/vnd.text.list");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QList<QUrl> filenames;

    while(!stream.atEnd()) {
        QString text;
        stream >> text;
        filenames << text;
    }

    return insertTracks(row, filenames);
}

bool PlaylistModel::moveRows(const QModelIndex&, int sourceRow, int count,
                             const QModelIndex&, int destinationChild)
{
    if(sourceRow == destinationChild)
        return false;
    Q_ASSERT(destinationChild >= 0);

    if(!beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), destinationChild))
        return false;

    std::vector<Core::Playlist::value_type> tmp(playlist.getTracks(sourceRow, sourceRow + count));

    playlist.erase(sourceRow, sourceRow + count);
    auto s = playlist.insert(destinationChild < sourceRow ? destinationChild : destinationChild - count, tmp);

    endMoveRows();

    return count == s;
}

bool PlaylistModel::moveRows(int sourceRow, int count, int destinationChild) {
    return moveRows(QModelIndex(), sourceRow, count, QModelIndex(), destinationChild);
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex&) {
    if(row < 0 && (count + row) > rowCount())
        return false;
    TRACE_LOG(logject) << "removing tracks " << row << " - " << row+count;

    const auto s = playlist.size();
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    playlist.erase(row, row + count);
    endRemoveRows();
    assert(playlist.size() == s - count);

    m_duration = chrono::duration_cast<chrono::seconds>(this->playlist.duration()).count();
    Q_EMIT durationChanged(m_duration);

    return playlist.size() == s - count;
}

long PlaylistModel::duration() const {
    return m_duration;
}

TagBinding::TagBinding(QObject* parent) :
    QObject(parent)
{}

TagBinding::~TagBinding() {
    conn.disconnect();
}

void TagBinding::setTarget(const QQmlProperty& property) {
    auto context = QQmlEngine::contextForObject(property.object());
    assert(context);
    auto v = context->contextProperty("playlistModel");
    assert(v.isValid());
    PlaylistModel* pm = qvariant_cast<PlaylistModel*>(v);
    assert(pm);
    v = context->contextProperty("model");
    assert(v.isValid());
    v = qvariant_cast<QObject*>(v)->property("index");
    assert(v.isValid());
    bool b;
    auto index = v.toInt(&b);
    assert(b);
    auto track = pm->playlist.getTrack(index);
    assert(track);
    m_target_property = property;
    auto t = track->format_string(m_format_string.toStdString());
    m_target_property.write(QString::fromStdString(t ? *t : "?"));
//    conn = track->getTagsChangedSignal().connect([this, track] (const TagLib::PropertyMap&) {
//        auto t = track->format_string(m_format_string.toStdString());
//        m_target_property.write(QString::fromStdString(t ? *t : "?"));
//    });
}

} // namespace Melosic
