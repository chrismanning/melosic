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

#include <asio/post.hpp>

#include <melosic/core/track.hpp>
#include <melosic/melin/kernel.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/common/signal_core.hpp>
#include <melosic/common/optional.hpp>
#include <melosic/melin/library.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/melin/input.hpp>
#include <melosic/executors/default_executor.hpp>

#include "jsondocmodel.hpp"

#include <jbson/builder.hpp>

#include <taglib/tpropertymap.h>

#include "playlistmodel.hpp"

namespace Melosic {
namespace fs = boost::filesystem;

Logger::Logger PlaylistModel::logject(logging::keywords::channel = "PlaylistModel");

PlaylistModel::PlaylistModel(Core::Playlist playlist,
                             const std::shared_ptr<Decoder::Manager>& _decman,
                             const std::shared_ptr<Library::Manager>& _libman, QObject* parent)
    : QAbstractListModel(parent),
      m_playlist(playlist),
      decman(_decman),
      libman(_libman)
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
            auto text = data(index, TrackRoles::FilePath).toString();
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
    Refresher(Core::Playlist p, int start, int end, int stride = 1) noexcept
        :
          p(p),
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
            asio::post(Refresher(p, start, end, stride));
    }

    mutable Core::Playlist p;
    mutable int start;
    mutable int end;
    mutable int stride;
};

static network::uri to_uri(QUrl url) {
    std::error_code ec;
    auto str = QUrl::toPercentEncoding(url.toString(), ":/", "()");
    auto uri = network::make_uri(std::begin(str), std::end(str), ec);
    assert(ec ? uri.empty() : !uri.empty());

    network::uri_builder build(uri);
    build.authority("");
    uri = build.uri();
    return std::move(uri);
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
        if(auto qobj = qobject_cast<QItemSelectionModel*>(obj)) {
            TRACE_LOG(logject) << "qobj is QItemSelectionModel*";
            return insertTracks(row, QVariant::fromValue(qobj->selectedIndexes()));
        }
        else if(auto qobj = qobject_cast<JsonDocModel*>(obj)) {
            TRACE_LOG(logject) << "qobj is JsonDocModel*";
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

    if(row < 0)
        row = m_playlist.size();
    else if(row > m_playlist.size())
        row = m_playlist.size();

    const auto old_rowcount = rowCount();
    std::vector<Core::Track> tracks;
    for(auto&& var : var_list) {
        if(var.canConvert<QUrl>()) {
            auto uri = to_uri(var.toUrl());

            for(auto&& t : decman->tracks(uri))
                tracks.push_back(std::move(t));
        }
        else if(var.canConvert<QModelIndex>()) {
            auto idx = var.value<QModelIndex>();
            auto obj = idx.model();
            if(qobject_cast<const JsonDocModel*>(obj) != nullptr) {
                auto doc_var = idx.data(JsonDocModel::DocumentRole);
                const auto doc_map = doc_var.value<QVariantMap>();
                const auto end = doc_map.end();
#ifndef NDEBUG
                auto json = QJsonDocument::fromVariant(doc_map).toJson(QJsonDocument::Compact);
                TRACE_LOG(logject) << "From JSON: " << json.data();
#endif

                auto query = jbson::builder{};
                auto and_query = jbson::array_builder{};

                std::vector<jbson::document> track_docs;
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
                            if(i.key() == "_id")
                                continue;
                            auto metadata_query = jbson::builder{};

                            auto str = i.key().toStdString();
                            metadata_query.emplace("key", std::string_view(str));
                            str = i.value().toString().toStdString();
                            metadata_query.emplace("value", std::string_view(str));

                            and_query.emplace(jbson::builder("metadata", jbson::builder
                                                             ("$elemMatch", std::move(metadata_query))
                                                            ));
                        }
                        query("$and", jbson::element_type::array_element, and_query);
                    }
                    track_docs = libman->query(std::move(query));
                }
                catch(...) {
                    ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
                }

                for(auto&& track_doc : track_docs) {
                    try {
                        tracks.emplace_back(track_doc);
                    }
                    catch(...) {
                        ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
                    }
                }
            }
        }
        else {
            TRACE_LOG(logject) << "Unsupported file type: " << var.typeName();
        }
    }

    if(tracks.empty()) {
        ERROR_LOG(logject) << "No tracks to insert";
        return false;
    }

    TRACE_LOG(logject) << "Inserting " << tracks.size() << " tracks";
    beginInsertRows(QModelIndex(), row, row + tracks.size() -1);
    m_playlist.insert(row, tracks);
    endInsertRows();

    Q_EMIT dataChanged(index(row), index(row + tracks.size()-1));
//    m_kernel.getThreadManager().enqueue(Refresher(m_playlist, m_kernel.getThreadManager(), row, row+tracks.size(), 1));

    return rowCount() > old_rowcount;
}

void PlaylistModel::refreshTags(int start, int end) {
    assert(start >= 0);
    TRACE_LOG(logject) << "refreshing tags of tracks " << start << " - " << end;
    end = end < start ? m_playlist.size() : end+1;
//    for(auto& t : m_playlist.getTracks(start, end)) {
//        try {
//            t.reOpen();
//            t.close();
//        }
//        catch(...) {}
//    }
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
    assert(destinationChild >= 0);

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
