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
#include <QQmlComponent>

#include <melosic/melin/library.hpp>
#include <ejpp/ejdb.hpp>
#include <jbson/element.hpp>

#include "jsondocmodel.hpp"
#include "librarymanager.hpp"
#include "selectionmodel.hpp"

#ifdef QT_GUI_EXPORTS
#define QT_GUI_EXPORT BOOST_SYMBOL_EXPORT
#else
#define QT_GUI_EXPORT BOOST_SYMBOL_IMPORT
#endif

namespace Melosic {

class QT_GUI_EXPORT FilterPane : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString header MEMBER m_header NOTIFY headerChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate MEMBER m_delegate NOTIFY delegateChanged FINAL)

    Q_PROPERTY(QVariant unknownQuery READ unknownQuery WRITE setUnknownQuery NOTIFY unknownQueryChanged FINAL)

    Q_PROPERTY(QJSValue queryGenerator READ queryGenerator WRITE setQueryGenerator NOTIFY queryGeneratorChanged FINAL)
    Q_PROPERTY(QVariant generatedQuery READ generatedQuery NOTIFY queryGenerated FINAL)

    Q_PROPERTY(QVariant generatorPaths READ generatorPaths WRITE setGeneratorPaths NOTIFY generatorPathsChanged FINAL)

    Q_PROPERTY(QVariant query READ query WRITE setQuery NOTIFY queryChanged FINAL)
    Q_PROPERTY(QVariant paths READ paths WRITE setPaths NOTIFY pathsChanged FINAL)

    Q_PROPERTY(QVariantList sortFields READ sortFields WRITE setSortFields NOTIFY sortFieldsChanged FINAL)

    Q_PROPERTY(Melosic::FilterPane* dependsOn READ dependsOn WRITE setDependsOn NOTIFY dependsOnChanged FINAL)

    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT FINAL)
    Q_PROPERTY(Melosic::SelectionModel* selectionModel READ selectionModel CONSTANT FINAL)

    QString m_header;
    QQmlComponent* m_delegate{nullptr};

    struct impl;
    std::unique_ptr<impl> pimpl;

public:
    explicit FilterPane(ejdb::db, ejdb::collection, QObject* parent = nullptr);
    explicit FilterPane(QObject* parent = nullptr);

    virtual ~FilterPane();

    QAbstractItemModel* model() const;

    QVariant unknownQuery() const;
    void setUnknownQuery(QVariant);

    QJSValue queryGenerator() const;
    void setQueryGenerator(QJSValue);
    void setQueryGenerator(std::function<jbson::document(const std::vector<jbson::document_set>&)>);

    QVariant generatorPaths() const;
    void setGeneratorPaths(QVariant);

    QVariant generatedQuery() const;

    QVariant query() const;
    void setQuery(QVariant);

    QVariant paths() const;
    void setPaths(QVariant);

    QVariantList sortFields() const;
    void setSortFields(QVariantList);

    FilterPane* dependsOn() const;
    void setDependsOn(FilterPane*);

    SelectionModel* selectionModel() const;

Q_SIGNALS:
    void unknownQueryChanged(QVariant);
    void queryGeneratorChanged(QJSValue);
    void generatorPathsChanged(QVariant);
    void queryGenerated(QVariant);
    void queryChanged(QVariant);
    void headerChanged(QString);
    void pathsChanged(QVariant);
    void sortFieldsChanged(QVariantList);
    void dependsOnChanged(Melosic::FilterPane*);
    void delegateChanged(QQmlComponent*);
};

} // namespace Melosic
QML_DECLARE_TYPE(Melosic::FilterPane)

#endif // MELOSIC_FILTERPANE_HPP
