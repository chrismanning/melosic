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

#ifndef MELOSIC_JSON_DOC_MODEL_HPP
#define MELOSIC_JSON_DOC_MODEL_HPP

#include <vector>

#include <QAbstractListModel>

#include <jbson/document_fwd.hpp>
#include <jbson/element_fwd.hpp>

namespace Melosic {

class JsonDocModel : public QAbstractListModel {
    std::vector<jbson::document> m_docs;

public:
    explicit JsonDocModel(QObject *parent = nullptr);

    enum Roles { DocumentStringRole = Qt::UserRole * 7, DocumentRole };

    void setDocs(std::vector<jbson::document>&&);
    void setDocs(const std::vector<jbson::document>&);

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};

} // namespace Melosic

#endif // MELOSIC_JSON_DOC_MODEL_HPP
