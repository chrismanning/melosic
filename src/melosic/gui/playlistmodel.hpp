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
#include <chrono>
namespace chrono = std::chrono;

#include <QAbstractListModel>
#include <QStringList>
#include <QQmlListProperty>
#include <QUrl>
#include <QHash>
#include <QQmlPropertyValueSource>
#include <QQmlProperty>
#include <QSequentialIterable>

#include <network/uri.hpp>

#include <melosic/melin/logging.hpp>
#include <melosic/common/error.hpp>
#include <melosic/common/range.hpp>
#include <melosic/common/connection.hpp>

#include <jbson/document_fwd.hpp>

namespace Melosic {

struct TrackRoles {
    enum { FileName = Qt::UserRole + 1, FilePath, FileExtension, TagsReadable, Current, Duration };
};

namespace Core {
class Playlist;
}
namespace Playlist {
class Manager;
}
namespace Decoder {
class Manager;
}
namespace Library {
class Manager;
}

class PlaylistModel : public QAbstractListModel {
    Q_OBJECT
    Core::Playlist m_playlist;
    std::shared_ptr<Decoder::Manager> decman;
    std::shared_ptr<Library::Manager> libman;
    static Logger::Logger logject;
    long m_duration{0};

    Q_PROPERTY(long duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

    friend class CategoryTag;
    friend class TagBinding;

  public:
    explicit PlaylistModel(Core::Playlist m_playlist, const std::shared_ptr<Decoder::Manager>&,
                           const std::shared_ptr<Library::Manager>&, QObject* parent = nullptr);

    QString name() const;
    void setName(QString name);

    Q_INVOKABLE int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                      const QModelIndex& parent) override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool insertTracks(int row, QVariant);
    bool insertTracks(int row, QSequentialIterable);

    Q_INVOKABLE void refreshTags(int start, int end = -1);

    Q_INVOKABLE bool moveRows(const QModelIndex&, int sourceRow, int count, const QModelIndex&,
                              int destinationChild) override;
    Q_INVOKABLE bool moveRows(int sourceRow, int count, int destinationChild);
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    long duration() const;

Q_SIGNALS:
    void durationChanged(long);
    void nameChanged(QString);
};

class TagBinding : public QObject, public QQmlPropertyValueSource {
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueSource)

    Q_PROPERTY(QString formatString MEMBER m_format_string NOTIFY formatStringChanged)
    QString m_format_string;
    Signals::Connection conn;
    QQmlProperty m_target_property;

  public:
    explicit TagBinding(QObject* parent = nullptr);
    virtual ~TagBinding();

    void setTarget(const QQmlProperty& property) override;

Q_SIGNALS:
    void formatStringChanged(QString);
};

} // namespace Melosic

#endif // PLAYLISTMODEL_HPP
