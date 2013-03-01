/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#include <melosic/gui/playlistmodel.hpp>

#include "playlistcategorymodel.hpp"

PlaylistCategoryModel::PlaylistCategoryModel(QObject* parent)
    : KCategorizedSortFilterProxyModel(parent)
{}

bool PlaylistCategoryModel::subSortLessThan(const QModelIndex& left, const QModelIndex& right) const {
    return left.data(Melosic::TrackRoles::TrackNumber).toString() <
            right.data(Melosic::TrackRoles::TrackNumber).toString();
}
