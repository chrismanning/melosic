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

#ifndef MELOSIC_CATEGORYTAG_HPP
#define MELOSIC_CATEGORYTAG_HPP

#include <QObject>
#include <qqml.h>
#include <QModelIndex>

#include "category.hpp"

namespace Melosic {

class PlaylistModel;

class CategoryTag : public Criterion {
    Q_OBJECT
    QString m_field;
    Q_PROPERTY(QString field MEMBER m_field NOTIFY fieldChanged FINAL)

    PlaylistModel* m_playlist_model{nullptr};

  public:
    explicit CategoryTag(QObject* parent = nullptr);

    QString result(const QModelIndex&) const override;

Q_SIGNALS:
    void fieldChanged(QString);
};

} // Melosic

QML_DECLARE_TYPE(Melosic::CategoryTag)

#endif // MELOSIC_CATEGORYTAG_HPP
