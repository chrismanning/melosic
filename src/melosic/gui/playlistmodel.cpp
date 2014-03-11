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
#include <QItemSelectionModel>
#include <QJsonDocument>

#include <melosic/core/track.hpp>
#include <melosic/melin/kernel.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/common/signal_core.hpp>
#include <melosic/common/thread.hpp>
#include <melosic/common/optional.hpp>
#include <melosic/melin/library.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/melin/input.hpp>

#include "jsondocmodel.hpp"

#include <jbson/builder.hpp>

#include <taglib/tpropertymap.h>

#include "playlistmodel.hpp"

namespace Melosic {
namespace fs = boost::filesystem;

Logger::Logger PlaylistModel::logject(logging::keywords::channel = "PlaylistModel");

PlaylistModel::PlaylistModel(Core::Playlist playlist, Core::Kernel& k, QObject* parent)
    : QAbstractListModel(parent),
      m_playlist(playlist),
      m_kernel(k)
{
    this->m_playlist.getMutlipleTagsChangedSignal().connect([this] (int start, int end) {
        Q_EMIT dataChanged(index(start), index(end-1));
    });

    connect(this, &PlaylistModel::dataChanged, [this] (const QModelIndex&, const QModelIndex&, const QVector<int>&) {
        m_duration = chrono::duration_cast<chrono::seconds>(this->m_playlist.duration()).count();
        Q_EMIT durationChanged(m_duration);
    });
}

QString PlaylistModel::name() const {
    return QString::fromLocal8Bit(m_playlist.name().data(), m_playlist.name().size());
}

void PlaylistModel::setName(QString name) {
    m_playlist.name(name.toStdString());
    Q_EMIT nameChanged(name);
}

int PlaylistModel::rowCount(const QModelIndex& /*parent*/) const {
    return m_playlist.size();
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid())
        return {};

    if(index.row() >= m_playlist.size())
        return {};

    try {
        auto track = m_playlist.getTrack(index.row());
        if(!track)
            return {};
        switch(role) {
            case Qt::DisplayRole:
                return data(index, TrackRoles::FileName);
            case TrackRoles::FileName:
                return QString::fromUtf8(Input::uri_to_path(track->uri()).filename().c_str());
            case TrackRoles::FilePath:
                return QString::fromUtf8(Input::uri_to_path(track->uri()).c_str());
            case TrackRoles::FileExtension:
                return QString::fromUtf8(Input::uri_to_path(track->uri()).extension().c_str());
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

    void operator()() const {
        while(start+stride > end)
            --stride;
        p.refreshTracks(start, start+stride);
        start += stride;
        if(start < end)
            tman.enqueue(Refresher(p, tman, start, end, stride));
    }

    mutable Core::Playlist p;
    Thread::Manager& tman;
    mutable int start;
    mutable int end;
    mutable int stride;
};

static network::uri to_uri(QUrl url) {
    std::error_code ec;
    auto str = url.toEncoded();
    auto uri = network::make_uri(std::begin(str), std::end(str), ec);
    network::uri_builder build(uri);
    build.authority("");
    uri = build.uri();
    assert(ec ? uri.empty() : !uri.empty());
    return uri;
}

bool PlaylistModel::insertTracks(int row, QVariant var) {
    TRACE_LOG(logject) << "In insertTracks(int, QVariant)";
    TRACE_LOG(logject) << "QVariant type: " << var.typeName();
    if(var.canConvert<QVariantList>()) {
        TRACE_LOG(logject) << "var is list";
        return insertTracks(row, var.value<QSequentialIterable>());
    }
    else if(static_cast<QMetaType::Type>(var.type()) == QMetaType::QObjectStar) {
        TRACE_LOG(logject) << "var is QObject*";
        auto obj = var.value<QObject*>();
        assert(obj != nullptr);
        if(auto qobj = qobject_cast<QItemSelectionModel*>(obj))
            return insertTracks(row, QVariant::fromValue(qobj->selectedIndexes()));
        else if(auto qobj = qobject_cast<JsonDocModel*>(obj)) {
            QModelIndexList items;
            for(auto i = 0; i < qobj->rowCount(); i++)
                items.append(qobj->index(i));
            return insertTracks(row, QVariant::fromValue(items));
        }
    }
    return false;
}

bool PlaylistModel::insertTracks(int row, QSequentialIterable var_list) {
    TRACE_LOG(logject) << "In insertTracks(int, QSequentialIterable)";
    TRACE_LOG(logject) << "Inserting " << var_list.size() << " files";

    const auto old_rowcount = rowCount();
    for(auto&& var : var_list) {
        if(var.canConvert<QUrl>()) {
            auto uri = to_uri(var.toUrl());
            row = ++row > m_playlist.size() ? m_playlist.size() : row;

            auto tracks = m_kernel.getDecoderManager().tracks(uri);
            TRACE_LOG(logject) << tracks.size() << " tracks inserting";

            beginInsertRows(QModelIndex(), row, row + boost::distance(tracks) -1);

            auto n = m_playlist.insert(row, tracks);
            TRACE_LOG(logject) << n << " tracks inserted";

            endInsertRows();

            m_kernel.getThreadManager().enqueue(Refresher(m_playlist, m_kernel.getThreadManager(), row, row+n, 1));
            row += n;
        }
        else if(var.canConvert<QModelIndex>()) {
            auto idx = var.value<QModelIndex>();
            auto obj = idx.model();
            row = ++row > m_playlist.size() ? m_playlist.size() : row;
            if(qobject_cast<const JsonDocModel*>(obj) != nullptr) {
                auto doc_var = idx.data(JsonDocModel::DocumentRole);
                auto& lm = m_kernel.getLibraryManager();
                const auto doc_map = doc_var.value<QVariantMap>();
                const auto end = doc_map.end();
#ifndef NDEBUG
                auto json = QJsonDocument::fromVariant(doc_map).toJson(QJsonDocument::Compact);
                TRACE_LOG(logject) << json.data();
#endif

                auto query = jbson::builder{};
                auto metadata_query = jbson::builder{};

                std::vector<jbson::document> tracks;
                try {
                    // build query
                    auto it = doc_map.find("location");
                    if(it != doc_map.end()) {
                        query(it.key().toStdString(), jbson::element_type::string_element,
                              it.value().toString().toStdString());
                    }
                    else {
                        for(auto i = doc_map.begin(); i != end; ++i) {
                            assert(i.key() != "_id");
                            auto str = i.key().toStdString();
                            metadata_query.emplace("key", boost::string_ref(str));
                            str = i.value().toString().toStdString();
                            metadata_query.emplace("value", boost::string_ref(str));
                        }
                        query("metadata", jbson::element_type::document_element, metadata_query);
                    }
                    tracks = lm.query(std::move(query));
                }
                catch(...) {
                    ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
                }

                const auto old_row = row;
                for(auto&& track_doc : tracks) {
                    beginInsertRows(QModelIndex(), row, row);

                    m_playlist.insert(row++, Core::Track{track_doc});

                    endInsertRows();
                }
                m_kernel.getThreadManager().enqueue(Refresher(m_playlist,
                                                              m_kernel.getThreadManager(), old_row, row, 1));
            }
        }
    }
    return rowCount() > old_rowcount;
}

void PlaylistModel::refreshTags(int start, int end) {
    Q_ASSERT(start >= 0);
    TRACE_LOG(logject) << "refreshing tags of tracks " << start << " - " << end;
    end = end < start ? m_playlist.size() : end+1;
    for(auto& t : m_playlist.getTracks(start, end)) {
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

    return insertTracks(row, QVariant::fromValue(filenames));
}

bool PlaylistModel::moveRows(const QModelIndex&, int sourceRow, int count,
                             const QModelIndex&, int destinationChild)
{
    if(sourceRow == destinationChild)
        return false;
    Q_ASSERT(destinationChild >= 0);

    if(!beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), destinationChild))
        return false;

    std::vector<Core::Playlist::value_type> tmp(m_playlist.getTracks(sourceRow, sourceRow + count));

    m_playlist.erase(sourceRow, sourceRow + count);
    auto s = m_playlist.insert(destinationChild < sourceRow ? destinationChild : destinationChild - count, tmp);

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

    const auto s = m_playlist.size();
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    m_playlist.erase(row, row + count);
    endRemoveRows();
    assert(m_playlist.size() == s - count);

    m_duration = chrono::duration_cast<chrono::seconds>(this->m_playlist.duration()).count();
    Q_EMIT durationChanged(m_duration);

    return m_playlist.size() == s - count;
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
    auto track = pm->m_playlist.getTrack(index);
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
