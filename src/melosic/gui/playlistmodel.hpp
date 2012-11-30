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

#include <QAbstractListModel>
#include <QStringList>

#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/common/logging.hpp>
#include <melosic/common/error.hpp>
#include <melosic/common/file.hpp>

namespace Melosic {
struct DataRoles {
    enum {
        SourceName = 33,
        Track
    };
};
}

class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    boost::shared_ptr<Melosic::Playlist> playlist;
    Melosic::Logger::Logger logject;

public:
    explicit PlaylistModel(boost::shared_ptr<Melosic::Playlist> playlist, QObject* parent = 0);
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    QMimeData* mimeData(const QModelIndexList& indexes) const;

    template <typename Iterator>
    void appendTracks(Iterator first, Iterator last) {
        auto beg = playlist->end() - playlist->begin();
        beginInsertRows(QModelIndex(), beg, beg + std::distance(first, last));
        playlist->insert(playlist->end(), first, last);
        endInsertRows();
    }

    template <class StringList>
    QStringList appendFiles(const StringList& filenames) {
        auto beg = playlist->end() - playlist->begin();
        QStringList failList;
        beginInsertRows(QModelIndex(), beg, beg + filenames.size());
        for(const auto& filename : filenames) {
            try {
                playlist->emplace_back(filename);
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
        return failList;
    }

    template<typename ... Args>
    Melosic::Playlist::iterator emplace(Melosic::Playlist::iterator pos, Args&& ... args) {
        auto beg = pos-playlist->begin();
        beginInsertRows(QModelIndex(), beg, beg + 1);
        auto ret = playlist->emplace(pos, std::forward<Args>(args)...);
        endInsertRows();
        return ret;
    }
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
};

#endif // PLAYLISTMODEL_HPP
