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

#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>

#include "playlistmodel.hpp"
#include "categoryproxymodel.hpp"
#include "categorytag.hpp"

namespace Melosic {

CategoryTag::CategoryTag(QObject *parent) :
    Criteria(parent)
{
    connect(this, &Criteria::modelChanged, [this] (CategoryProxyModel* m) {
        if(!m)
            return;
        m_playlist_model = qobject_cast<PlaylistModel*>(m->sourceModel());
    });
}

QString CategoryTag::result(const QModelIndex& index) const {
    Q_ASSERT(index.isValid());
    if(!m_playlist_model)
        return m_field;
    auto t = m_playlist_model->playlist.getTrack(index.row());
    if(!t)
        return m_field;
    auto tag = t->getTag(m_field.toStdString());
    if(!tag)
        return m_field;
    return QString::fromStdString(*tag);
}

}
