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

#ifndef MELOSIC_PLAYLISTMANAGERMODEL_HPP
#define MELOSIC_PLAYLISTMANAGERMODEL_HPP

#include <QAbstractListModel>

#include <melosic/melin/logging.hpp>

namespace Melosic {

namespace Playlist {
class Manager;
}

class PlaylistModel;

class PlaylistManagerModel : public QAbstractListModel {
    Q_OBJECT
    Playlist::Manager& playman;
    QHash<QString, PlaylistModel*> playlists;
    Logger::Logger logject;

public:
    explicit PlaylistManagerModel(Playlist::Manager&, QObject* parent = nullptr);

    enum {
        PlaylistModelRole = Qt::UserRole * 12
    };

    Q_INVOKABLE QObject* playlist(QString name);

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    Q_INVOKABLE QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    Q_INVOKABLE bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Q_INVOKABLE bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    Q_INVOKABLE QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const override;
};

} // namespace Melosic

#endif // MELOSIC_PLAYLISTMANAGERMODEL_HPP
