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

#ifndef MELOSIC_SELECTIONMODEL_HPP
#define MELOSIC_SELECTIONMODEL_HPP

#include <QItemSelectionModel>
#include <qqml.h>

namespace Melosic {

class SelectionModel : public QItemSelectionModel {
    Q_OBJECT

    Q_PROPERTY(int currentRow READ currentRow WRITE setCurrentRow NOTIFY currentRowChanged FINAL)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged FINAL)

  public:
    explicit SelectionModel(QAbstractItemModel* model, QObject* parent = nullptr);

    using QItemSelectionModel::SelectionFlag;
    Q_ENUMS(SelectionFlag)
    using QItemSelectionModel::clear;
    using QItemSelectionModel::clearCurrentIndex;
    using QItemSelectionModel::clearSelection;
    using QItemSelectionModel::select;
    using QItemSelectionModel::setCurrentIndex;
    using QItemSelectionModel::isSelected;

    int currentRow() const;
    void setCurrentRow(int row);

Q_SIGNALS:
    void currentRowChanged(int row);
    void hasSelectionChanged(bool hasSelection);

  public Q_SLOTS:
    bool isSelected(int row) const;
    virtual void select(int row, QItemSelectionModel::SelectionFlags command);
    virtual void select(int from_row, int to_row, QItemSelectionModel::SelectionFlags command);
    virtual void setCurrentIndex(int row, QItemSelectionModel::SelectionFlags command);
};

} // namespace Melosic

QML_DECLARE_TYPE(Melosic::SelectionModel)

#endif // MELOSIC_SELECTIONMODEL_HPP
