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

#include <iostream>

#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QQmlContext>

#include <jbson/json_reader.hpp>
#include <jbson/path.hpp>
using namespace jbson::literal;

#include <melosic/melin/library.hpp>

#include "filterpane.hpp"

namespace Melosic {

FilterPane::FilterPane(QObject* parent) : QObject(parent) {
    m_model = new JsonDocModel(this);
    m_selection_model = new SelectionModel(m_model, this);
    connect(this, &FilterPane::queryChanged, [this] (auto&&) {
        std::clog << "queryChanged" << std::endl;
        if(m_paths.empty())
            return;
        refresh();
    });
    connect(this, &FilterPane::pathsChanged, [this] (auto&&) { refresh(); });
    connect(this, &FilterPane::dependsOnChanged, [this] (auto&&) {
        std::clog << "dependsOnChanged" << std::endl;
        if(m_depends == nullptr)
            return;
        auto sel_model = m_depends->m_selection_model;
        if(sel_model == nullptr)
            return;
        assert(sel_model->model() != nullptr);

        connect(sel_model, &QItemSelectionModel::selectionChanged, [this] (auto&& selected, auto&& deselected) {
            std::clog << "selection changed" << std::endl;
            for(auto&& range : selected) {
                for(auto&& i : range.indexes()) {
                    auto doc = i.data(JsonDocModel::DocumentRole);
                    jbson::json_reader reader;
                    auto str = doc.toString();
                    std::clog << "doc str: " << qPrintable(str) << std::endl;
                    reader.parse(str.isEmpty() || str.isNull() ? "{}": str.toUtf8());
                    std::clog << "depends path: " << qPrintable(m_depends_path) << std::endl;
                    auto res = jbson::path_select(jbson::document(std::move(reader)), m_depends_path.toUtf8());
                    std::clog << "res size: " << res.size() << std::endl;
                    for(auto&& val : res) {
                        if(val.type() != jbson::element_type::string_element)
                            continue;
                        m_depend_selection.push_back(QString::fromStdString(val.value<std::string>()));
                    }
                }
            }
            for(auto&& range : deselected) {
                for(auto&& i : range.indexes()) {
                    auto doc = i.data(JsonDocModel::DocumentRole);
                    jbson::json_reader reader;
                    auto str = doc.toString();
                    std::clog << "doc str: " << qPrintable(str) << std::endl;
                    reader.parse(str.isEmpty() || str.isNull() ? "{}": str.toUtf8());
                    std::clog << "depends path: " << qPrintable(m_depends_path) << std::endl;
                    auto res = jbson::path_select(jbson::document(std::move(reader)), m_depends_path.toUtf8());
                    std::clog << "res size: " << res.size() << std::endl;
                    for(auto&& val : res) {
                        if(val.type() != jbson::element_type::string_element)
                            continue;
                        auto it = boost::range::find(m_depend_selection, QString::fromStdString(val.value<std::string>()));
                        m_depend_selection.erase(it);
                    }
                }
            }
            std::clog << "m_depend_selection size: " << m_depend_selection.size() << std::endl;
            Q_EMIT dependSelectionChanged(m_depend_selection);
        });
        sel_model->clear();
    });
}

QVariant FilterPane::query() const { return m_query; }

std::ostream& operator<<(std::ostream& os, QVariantMap map) {
    os << "{ ";
    for(auto it = map.begin(); it != map.end();) {
        os << qPrintable(it.key()) << ": ";
        if(it.value().type() == QVariant::String)
            os << '"' << qPrintable(it.value().toString()) << '"';
        else if(it.value().canConvert<QVariantMap>())
            os << it.value().value<QVariantMap>();

        if(++it != map.end())
            os << ", ";
    }
    os << " }";
    return os;
}

void FilterPane::setQuery(QVariant q) {
//    if(q == m_query)
//        return;
    m_query = q;
    Q_EMIT queryChanged(m_query);
}

QVariantMap FilterPane::paths() const {
    return m_paths;
}

void FilterPane::setPaths(QVariantMap p) {
    if(p == m_paths)
        return;
    m_paths = p;
    Q_EMIT pathsChanged(p);
}

FilterPane* FilterPane::dependsOn() const {
    return m_depends;
}

void FilterPane::setDependsOn(FilterPane* d) {
    if(d == m_depends)
        return;
    m_depends = d;
    Q_EMIT dependsOnChanged(m_depends);
}

QVariantList FilterPane::dependSelection() const {
    return m_depend_selection;
}

QString FilterPane::dependsPath() const { return m_depends_path; }

void FilterPane::setDependsPath(QString str) { m_depends_path = str; }

SelectionModel* FilterPane::selectionModel() const {
    return m_selection_model;
}

void FilterPane::refresh() {
    if(libman == nullptr) {
        auto ctx = QQmlEngine::contextForObject(this);
        assert(ctx);
        auto v = ctx->contextProperty("LibraryManager");
        assert(v.isValid());
        libman = v.value<LibraryManager*>();
    }
    assert(libman != nullptr);

    if(!m_query.isValid() || m_query.isNull())
        return;

    auto& lm = libman->getLibraryManager();

    QString json_string;
    if(m_query.type() == QVariant::String)
        json_string = m_query.toString();
    else
        json_string = QString(QJsonDocument::fromVariant(m_query).toJson());
    std::clog << qPrintable(objectName()) << " query: " << qPrintable(json_string) << std::endl;

    jbson::document qry;
    try {
        jbson::json_reader reader;
        reader.parse(json_string.toUtf8());
        qry = std::move(reader);
    }
    catch(...) {
        std::clog << boost::current_exception_diagnostic_information();
        qry = "{}"_json_doc;
    }

    std::vector<jbson::document> docs;
    if(m_paths.empty())
        docs = lm.query(qry);
    else {
        auto sp = std::vector<std::tuple<std::string, std::string>>{};
        for(auto it = m_paths.begin(); it != m_paths.end(); ++it) {
            assert(it->isValid());
            assert(it->type() == QVariant::String);
            sp.emplace_back(it.key().toStdString(), it->toString().toStdString());
        }
        auto ds = lm.query(qry, sp);
        boost::range::sort(ds);
        std::transform(ds.begin(), std::unique(ds.begin(), ds.end()),
                       std::back_inserter(docs), [](auto&& d) { return jbson::document{d}; });
    }
    std::clog << "no. results: " << docs.size() << std::endl;

    //TODO: replicate selection in new results
    m_selection_model->clear();
    m_model->setDocs(std::move(docs));
}

} // namespace Melosic
