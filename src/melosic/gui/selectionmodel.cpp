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

#include "selectionmodel.hpp"

namespace Melosic {

SelectionModel::SelectionModel(QAbstractItemModel* model, QObject* parent) : QItemSelectionModel(model, parent) {
    connect(this, &SelectionModel::currentChanged, [this] (auto&& cur, auto&&) {
        Q_EMIT currentRowChanged(cur.row());
    });
}

int SelectionModel::currentRow() const {
    return currentIndex().row();
}

void SelectionModel::setCurrentRow(int row) {
    setCurrentIndex(row, SelectCurrent);
}

bool SelectionModel::isSelected(int row) const {
    Q_ASSERT(model() != nullptr);
    return isSelected(model()->index(row, 0));
}

void SelectionModel::select(int row, QItemSelectionModel::SelectionFlags command) {
    Q_ASSERT(model() != nullptr);
    select(model()->index(row, 0), command);
}

void SelectionModel::select(int from_row, int to_row, QItemSelectionModel::SelectionFlags command) {
    Q_ASSERT(model() != nullptr);
    select(QItemSelection(model()->index(from_row, 0), model()->index(to_row, 0)), command);
}

void SelectionModel::setCurrentIndex(int row, QItemSelectionModel::SelectionFlags command)
{
    Q_ASSERT(model() != nullptr);
    setCurrentIndex(model()->index(row, 0), command);
}

} // namespace Melosic
