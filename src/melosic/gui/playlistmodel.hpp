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

#ifndef PLAYLISTMODEL_HPP
#define PLAYLISTMODEL_HPP

#include <memory>
#include <initializer_list>

#include <QAbstractListModel>
#include <QStringList>
#include <QQmlListProperty>
#include <QUrl>
#include <QHash>

#include <boost/filesystem/path.hpp>

#include <melosic/melin/logging.hpp>
#include <melosic/common/error.hpp>
#include <melosic/common/range.hpp>
#include <melosic/common/connection.hpp>

namespace Melosic {

struct TrackRoles {
    enum {
        FileName = Qt::UserRole + 1,
        FilePath,
        FileExtension,
        TagsReadable,
        Current,
        Title,
        Artist,
        Album,
        AlbumArtist,
        TrackNumber,
        Genre,
        Date,
        Duration
    };
};

namespace Core {
class Playlist;
}
namespace Thread {
class Manager;
}

class PlaylistModel : public QAbstractListModel {
    Q_OBJECT
    Core::Playlist playlist;
    Thread::Manager& tman;
    static Logger::Logger logject;

public:
    explicit PlaylistModel(Core::Playlist playlist, Thread::Manager&,
                           QObject* parent = nullptr);

    Q_INVOKABLE int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data,
                      Qt::DropAction action,
                      int row, int column,
                      const QModelIndex& parent) override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool insertTracks(int row, QList<QUrl>);
    bool insertTracks(int row, ForwardRange<const boost::filesystem::path>);
    bool insertTracks(int row, std::initializer_list<const boost::filesystem::path> filenames) {
        return insertTracks(row, std::move(ForwardRange<const boost::filesystem::path>(filenames)));
    }

    Q_INVOKABLE void refreshTags(int start, int end = -1);

    Q_INVOKABLE bool moveRows(const QModelIndex&, int sourceRow, int count,
                              const QModelIndex&, int destinationChild) override;
    Q_INVOKABLE bool moveRows(int sourceRow, int count, int destinationChild);
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
};

} // namespace Melosic

#endif // PLAYLISTMODEL_HPP
