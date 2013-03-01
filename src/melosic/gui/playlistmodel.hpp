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
#include <kcategorydrawer.h>

#include <boost/filesystem/path.hpp>

#include <opqit/opaque_iterator.hpp>

#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/error.hpp>
#include <melosic/common/file.hpp>
#include <melosic/common/range.hpp>

namespace Melosic {
struct TrackRoles {
    enum {
        SourceName = Qt::UserRole + 1,
        Category,
        Title,
        Artist,
        Album,
        AlbumArtist,
        TrackNumber,
        Genre,
        Date,
        Length
    };
};
class Kernel;
}

class PlaylistModel : public QAbstractListModel {
    Q_OBJECT
    std::shared_ptr<Melosic::Playlist> playlist;
    Melosic::Kernel& kernel;
    Melosic::Logger::Logger logject;

public:
    PlaylistModel(Melosic::Kernel& kernel,
                  std::shared_ptr<Melosic::Playlist> playlist,
                  QObject* parent = 0);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    virtual bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data,
                      Qt::DropAction action,
                      int row, int column,
                      const QModelIndex& parent) override;
    QHash<int, QByteArray> roleNames() const override;

    void appendTracks(const Melosic::ForwardRange<Melosic::Track>& tracks);
    QStringList appendFiles(const Melosic::ForwardRange<const boost::filesystem::path>&);
    QStringList appendFiles(std::initializer_list<const boost::filesystem::path> filenames) {
        return appendFiles(Melosic::ForwardRange<const boost::filesystem::path>(filenames));
    }

    template<typename ... Args>
    Melosic::Playlist::iterator emplace(Melosic::Playlist::iterator pos, Args&& ... args) {
        auto beg = pos-playlist->begin();
        beginInsertRows(QModelIndex(), beg, beg + 1);
        auto ret = playlist->insert(pos, Melosic::Track(std::forward<Args>(args)...));
        endInsertRows();
        return ret;
    }
};

class CategoryDrawer : public KCategoryDrawerV3 {
public:
    CategoryDrawer(KCategorizedView*);
    virtual ~CategoryDrawer() {}

    virtual void drawCategory(const QModelIndex& index,
                              int sortRole,
                              const QStyleOption& option,
                              QPainter* painter) const override;
};

#endif // PLAYLISTMODEL_HPP
