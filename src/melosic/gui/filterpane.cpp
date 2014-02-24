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

#include <boost/range/adaptor/transformed.hpp>

#include <melosic/melin/library.hpp>

#include "filterpane.hpp"

namespace Melosic {

FilterPane::FilterPane(QObject* parent) : QObject(parent) {
    m_model = new JsonDocModel(this);
    m_selection_model = new SelectionModel(m_model, this);
    connect(this, &FilterPane::queryChanged, [this] (auto&&) {
        if(m_paths.empty())
            return;
        refresh();
    });
    connect(this, &FilterPane::pathsChanged, [this] (auto&&) { refresh(); });
    connect(this, &FilterPane::dependsOnChanged, [this] (auto&&) {
        if(m_depends == nullptr)
            return;
        auto sel_model = m_depends->m_selection_model;
        if(sel_model == nullptr)
            return;
        assert(sel_model->model() != nullptr);

        connect(sel_model, &QItemSelectionModel::selectionChanged, [this] (auto&& selected, auto&& deselected) {
            for(auto&& range : selected) {
                for(auto&& i : range.indexes()) {
                    auto doc = i.data(JsonDocModel::DocumentStringRole);
                    auto str = doc.toString();
                    if(str.isEmpty() || str.isNull())
                        continue;
                    jbson::json_reader reader;
                    reader.parse(str.toUtf8());
                    auto res = jbson::path_select(jbson::document(std::move(reader)), m_depends_path.toUtf8());
                    if(res.empty()) {
                        m_depend_selection.push_back("");
                        continue;
                    }
                    for(auto&& val : res) {
                        if(val.type() != jbson::element_type::string_element)
                            continue;
                        m_depend_selection.push_back(QString::fromStdString(val.value<std::string>()));
                    }
                }
            }
            for(auto&& range : deselected) {
                for(auto&& i : range.indexes()) {
                    auto doc = i.data(JsonDocModel::DocumentStringRole);
                    jbson::json_reader reader;
                    auto str = doc.toString();
                    reader.parse(str.isEmpty() || str.isNull() ? "{}": str.toUtf8());
                    auto res = jbson::path_select(jbson::document(std::move(reader)), m_depends_path.toUtf8());
                    if(res.empty()) {
                        m_depend_selection.erase(boost::range::find(m_depend_selection, ""));
                        continue;
                    }
                    for(auto&& val : res) {
                        if(val.type() != jbson::element_type::string_element)
                            continue;
                        auto it = boost::range::find(m_depend_selection, QString::fromStdString(val.value<std::string>()));
                        m_depend_selection.erase(it);
                    }
                }
            }
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

static Library::Manager& libman(const QObject* obj) {
    auto ctx = QQmlEngine::contextForObject(obj);
    assert(ctx);
    auto v = ctx->contextProperty("LibraryManager");
    assert(v.isValid());
    auto libman = v.value<LibraryManager*>();
    assert(libman != nullptr);

    return libman->getLibraryManager();
}

template <typename AssocT, typename ValueT>
auto find_optional(AssocT&& rng, ValueT&& val) {
    boost::optional<typename boost::range_value<std::decay_t<AssocT>>::type> ret;
    auto it = rng.find(val);
    if(it != std::end(rng))
        return ret = *it;
    return ret = boost::none;
}

template <typename C1, typename C2>
static boost::optional<int> vers_cmp_optional(const boost::optional<jbson::basic_element<C1>>& a,
                                              const boost::optional<jbson::basic_element<C2>>& b) {
    if((!a || !b) || a->type() != b->type() || a->type() != jbson::element_type::string_element)
        return boost::none;
    return ::strverscmp(jbson::get<jbson::element_type::string_element>(*a).data(),
                        jbson::get<jbson::element_type::string_element>(*b).data());
}

template <typename C1, typename C2>
static bool contains_digit(const boost::optional<jbson::basic_element<C1>>& a,
                           const boost::optional<jbson::basic_element<C2>>& b) {
    if((!a || !b) || a->type() != b->type() || a->type() != jbson::element_type::string_element)
        return false;
    auto str1 = jbson::get<jbson::element_type::string_element>(*a);
    auto str2 = jbson::get<jbson::element_type::string_element>(*b);
    return std::any_of(std::begin(str1), std::end(str1), ::isdigit) &&
            std::any_of(std::begin(str2), std::end(str2), ::isdigit);
}

template <typename AssocRangeT, typename StringRangeT>
static inline void sort_by_criteria_impl(AssocRangeT& rng, const StringRangeT& sort_fields) {
    boost::range::sort(rng, [&](auto&& a, auto&& b) {
        for(const auto& sf: sort_fields) {
            auto val1 = find_optional(a, sf);
            auto val2 = find_optional(b, sf);
            if(val1 == val2)
                continue;
            if(contains_digit(val1, val2))
                return vers_cmp_optional(val1, val2) < 0;
            return val1 < val2;
        }
        return a < b;
    });
}

template <typename AssocRangeT, typename StringRangeT>
static void sort_by_criteria(AssocRangeT& rng, const StringRangeT& sort_fields) {
    sort_by_criteria_impl(rng, sort_fields);
}

template <typename AssocRangeT, typename StringT>
static void sort_by_criteria(AssocRangeT& rng, std::initializer_list<StringT> sort_fields) {
    sort_by_criteria_impl(rng, sort_fields);
}

void FilterPane::refresh() {
    if(!m_query.isValid() || m_query.isNull())
        return;

    auto& lm = libman(this);

    QString json_string;
    if(m_query.type() == QVariant::String)
        json_string = m_query.toString();
    else
        json_string = QString(QJsonDocument::fromVariant(m_query).toJson());

    jbson::document qry;
    try {
        jbson::json_reader reader;
        reader.parse(json_string.toUtf8());
        qry = std::move(reader);
    }
    catch(...) {
        std::clog << boost::current_exception_diagnostic_information();
        std::clog << "Using empty query" << std::endl;
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
        std::clog << qPrintable(this->objectName()) << ": query results: " << ds.size() << std::endl;
        sort_by_criteria(ds, boost::adaptors::transform(m_sort_fields, [](auto&& var) {
            auto bs = var.toString().toUtf8();
            return boost::string_ref(bs, bs.size());
        }));
        std::transform(ds.begin(), std::unique(ds.begin(), ds.end()),
                       std::back_inserter(docs), [](auto&& d) { return jbson::document{d}; });
        std::clog << qPrintable(this->objectName()) << ": filtered results: " << docs.size() << std::endl;
    }

    //TODO: replicate selection in new results
    assert(m_selection_model != nullptr);
    m_selection_model->clear();
    m_model->setDocs(std::move(docs));
}

} // namespace Melosic
