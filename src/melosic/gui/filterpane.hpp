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

#ifndef MELOSIC_FILTERPANE_HPP
#define MELOSIC_FILTERPANE_HPP

#include <QObject>
#include <qqml.h>
#include <QItemSelectionModel>

#include "jsondocmodel.hpp"
#include "librarymanager.hpp"
#include "selectionmodel.hpp"

namespace Melosic {

class FilterPane : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString header MEMBER m_header NOTIFY headerChanged FINAL)
    Q_PROPERTY(QVariant query READ query WRITE setQuery NOTIFY queryChanged FINAL)
    Q_PROPERTY(QVariantMap paths READ paths WRITE setPaths NOTIFY pathsChanged FINAL)

    Q_PROPERTY(Melosic::FilterPane* dependsOn READ dependsOn WRITE setDependsOn NOTIFY dependsOnChanged FINAL)
    Q_PROPERTY(QVariantList dependSelection READ dependSelection NOTIFY dependSelectionChanged FINAL)

    Q_PROPERTY(QString dependsPath READ dependsPath WRITE setDependsPath FINAL)
    Q_PROPERTY(QAbstractItemModel* model MEMBER m_model CONSTANT FINAL)
    Q_PROPERTY(Melosic::SelectionModel* selectionModel READ selectionModel CONSTANT FINAL)

    QString m_header;
    QVariant m_query;
    QVariantMap m_paths;
    FilterPane* m_depends;
    QVariantList m_depend_selection;
    QString m_depends_path;
    JsonDocModel* m_model;
    SelectionModel* m_selection_model;
    LibraryManager* libman;

public:
    explicit FilterPane(QObject* parent = nullptr);

    QVariant query() const;
    void setQuery(QVariant);

    QVariantMap paths() const;
    void setPaths(QVariantMap);

    FilterPane* dependsOn() const;
    void setDependsOn(FilterPane*);

    QVariantList dependSelection() const;

    QString dependsPath() const;
    void setDependsPath(QString);

    SelectionModel* selectionModel() const;

Q_SIGNALS:
    void queryChanged(QVariant);
    void headerChanged(QString);
    void pathsChanged(QVariantMap);
    void dependsOnChanged(Melosic::FilterPane*);
    void dependSelectionChanged(QVariantList);

public Q_SLOTS:
    void refresh();

private:
    static QVariant* at_dep_sel(QQmlListProperty<QVariant>* list, int index);
    static int count_dep_sel(QQmlListProperty<QVariant>* list);
};

} // namespace Melosic
QML_DECLARE_TYPE(Melosic::FilterPane)

#endif // MELOSIC_FILTERPANE_HPP
