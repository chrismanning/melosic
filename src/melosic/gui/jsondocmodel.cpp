/**************************************************************************
**  Copyright (C) 2014 Christian Manning
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

#include <jbson/document.hpp>
#include <jbson/json_writer.hpp>
#include <jbson/path.hpp>

#include "jsondocmodel.hpp"

namespace Melosic {

JsonDocModel::JsonDocModel(QObject* parent) : QAbstractListModel(parent) {}

void JsonDocModel::setDocs(std::vector<jbson::document>&& docs) {
    Q_EMIT beginResetModel();
    m_docs = std::move(docs);
    Q_EMIT endResetModel();
}

void JsonDocModel::setDocs(const std::vector<jbson::document>& docs) {
    Q_EMIT beginResetModel();
    m_docs = docs;
    Q_EMIT endResetModel();
}

Qt::ItemFlags JsonDocModel::flags(const QModelIndex& index) const { return QAbstractItemModel::flags(index); }

int JsonDocModel::rowCount(const QModelIndex&) const {
    if(m_docs.size() > std::numeric_limits<int>::max())
        return -1;
    return static_cast<int>(m_docs.size());
}

static QString toQString(boost::string_ref str) {
    return QString::fromUtf8(str.data(), str.size());
}

struct QVariantVisitor {
    QVariantMap& m_map;
    explicit QVariantVisitor(QVariantMap& map) : m_map(map) {}

    template <typename T>
    void operator()(boost::string_ref name, T v, jbson::element_type e, std::enable_if_t<std::is_arithmetic<T>::value>* = 0) {
        m_map.insert(toQString(name), QVariant::fromValue(v));
    }

    void operator()(boost::string_ref name, std::string str, jbson::element_type) {
        m_map.insert(toQString(name), QString::fromStdString(str));
    }

    void operator()(boost::string_ref name, boost::string_ref str, jbson::element_type) {
        m_map.insert(toQString(name), toQString(str));
    }

    template <typename T>
    void operator()(boost::string_ref name, jbson::basic_document<T>&& doc, jbson::element_type) {
        QVariantMap map{};
        QVariantVisitor v{map};
        for(auto&& e : doc)
            e.visit(v);
        m_map.insert(toQString(name), map);
    }

    template <typename T>
    void operator()(boost::string_ref, T&&, jbson::element_type, std::enable_if_t<!std::is_arithmetic<T>::value>* = 0) {
        assert(false);
    }
    void operator()(boost::string_ref, jbson::element_type) {
        assert(false);
    }
};

static QVariantMap doc_to_map(const jbson::document& doc) {
    QVariantMap map{};
    QVariantVisitor v{map};
    for(auto&& e : doc)
        e.visit(v);
    return std::move(map);
}

QVariant JsonDocModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid())
        return {};
    assert(index.row() < rowCount());

    switch(role) {
        case Qt::DisplayRole:
        case DocumentStringRole: {
            std::string str;
            jbson::write_json(m_docs[index.row()], std::back_inserter(str));
            return QString::fromStdString(str);
        }
        case DocumentRole:
            return doc_to_map(m_docs[index.row()]);
        default:
            break;
    }

    return QVariant{};
}

QVariant JsonDocModel::headerData(int section, Qt::Orientation orientation, int role) const {
    return QVariant{};
}

QHash<int, QByteArray> JsonDocModel::roleNames() const {
    auto roles = QAbstractListModel::roleNames();
    roles.insert(DocumentStringRole, "documentstring");
    roles.insert(DocumentRole, "document");
    return roles;
}

} // namespace Melosic
