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

static QVariantMap doc_to_map(const jbson::document&) {

}

QVariant JsonDocModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid()) {
        return {};
    }
    assert(index.row() < rowCount());

    switch(role) {
        case Qt::DisplayRole:
        case DocumentStringRole: {
            std::string str;
            jbson::write_json(m_docs[index.row()], std::back_inserter(str));
            return QString::fromStdString(str);
        }
        case DocumentRole: {
            QVariantMap map{};
            return map;
        }
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
