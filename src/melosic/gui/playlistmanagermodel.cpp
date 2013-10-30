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

#include <mutex>
using mutex = std::mutex;
using lock_guard = std::lock_guard<mutex>;
using unique_lock = std::unique_lock<mutex>;

#include <boost/optional.hpp>

#include <melosic/core/playlist.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/gui/playlistmodel.hpp>
#include <melosic/common/signal_core.hpp>

#include "playlistmanagermodel.hpp"

namespace Melosic {

PlaylistManagerModel::PlaylistManagerModel(Playlist::Manager& playman, Thread::Manager& tman, QObject* parent)
    : QAbstractListModel(parent),
      playman(playman),
      tman(tman),
      logject(logging::keywords::channel = "PlaylistManagerModel")
{
    conns.emplace_back(playman.getPlaylistAddedSignal().connect([this] (boost::optional<Core::Playlist> p) {
        lock_guard l(mu);
        TRACE_LOG(logject) << "Playlist added: " << !!p;
        if(p) {
           auto pm = new PlaylistModel(*p, this->tman);
           pm->moveToThread(this->thread());
           playlists.insert({*p, pm});
        }
    }));
    conns.emplace_back(playman.getPlaylistRemovedSignal().connect([this] (boost::optional<Core::Playlist> p) {
        lock_guard l(mu);
        TRACE_LOG(logject) << "Playlist removed: " << !!p;
        if(!p)
            return;
        auto it = playlists.left.find(*p);
        if(it != playlists.left.end())
            playlists.left.erase(it);
    }));
    conns.emplace_back(playman.getCurrentPlaylistChangedSignal().connect([this] (boost::optional<Core::Playlist> p) {
        if(!p) return;
        lock_guard l(mu);
        auto it = playlists.left.find(*p);
        if(it != playlists.left.end())
            setCurrentPlaylistModel(it->second);
    }));

    playman.insert(0, "Default");
}

Qt::ItemFlags PlaylistManagerModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

    if(index.isValid())
        return Qt::ItemIsEditable | defaultFlags;
    return defaultFlags;
}

QVariant PlaylistManagerModel::data(const QModelIndex& index, int role) const {
    lock_guard l(mu);
    assert(playman.size() == static_cast<Playlist::Manager::size_type>(playlists.size()));
    if(!index.isValid() || index.row() >= playman.size())
        return QVariant();

    assert(!playman.empty());
    switch(role) {
        case PlaylistTitleRole:
        case Qt::DisplayRole:
        case Qt::EditRole: {
            auto v = playman.getPlaylist(index.row());
            assert(v);
            return QString::fromStdString(v->getName());
        }
        case PlaylistModelRole: {
            auto v = playman.getPlaylist(index.row());
            assert(v);
            auto it = playlists.left.find(*v);
            assert(playlists.left.end() != it);

            return QVariant::fromValue(it->second);
        }
        default:
            return QVariant();
    }
}

QVariant PlaylistManagerModel::headerData(int, Qt::Orientation, int) const {
    return tr("Playlists");
}

QHash<int, QByteArray> PlaylistManagerModel::roleNames() const {
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles[PlaylistModelRole] = "playlistModel";
    roles[PlaylistTitleRole] = "title";
    return roles;
}

int PlaylistManagerModel::rowCount(const QModelIndex& /*parent*/) const {
    lock_guard l(mu);
    return playlists.size();
}

bool PlaylistManagerModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if(!index.isValid() || index.row() >= playman.size() || !(role & Qt::EditRole) || !value.isValid())
        return false;

    auto v = playman.getPlaylist(index.row());
    assert(v);
    v->setName(value.toString().toStdString());
    return true;
}

bool PlaylistManagerModel::insertRows(int row, int count, const QModelIndex&) {
    if(row > playman.size())
        return false;

    TRACE_LOG(logject) << "row: " << row << "; count: " << count;
    beginInsertRows(QModelIndex(), row, row+count-1);
    playman.insert(row, count);
    endInsertRows();
    return true;
}

bool PlaylistManagerModel::removeRows(int row, int count, const QModelIndex&) {
    playman.erase(row, count);
    return true;
}

QModelIndex PlaylistManagerModel::index(int row, int column, const QModelIndex& parent) const {
    return QAbstractListModel::index(row, column, parent);
}

PlaylistModel* PlaylistManagerModel::currentPlaylistModel() const {
    return m_current;
}

void PlaylistManagerModel::setCurrentPlaylistModel(PlaylistModel* pm) {
    if(m_current == pm)
        return;

    m_current = pm;

    auto it = playlists.right.find(m_current);
    if(it == playlists.right.end())
        return;
    playman.setCurrent(it->second);
    Q_EMIT currentPlaylistModelChanged(pm);
}

} // namespace Melosic
