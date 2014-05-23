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
#include <jbson/json_writer.hpp>
using namespace jbson::literal;

#include <boost/range/adaptor/transformed.hpp>

#include <melosic/melin/library.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/logging.hpp>

#include "filterpane.hpp"

namespace Melosic {

template <typename AssocT, typename ValueT> static auto find_optional(AssocT&& rng, ValueT&& val) {
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
        for(const auto& sf : sort_fields) {
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

static jbson::document to_document(const QVariant& var) {
    QByteArray json_string;
    if(var.type() == QVariant::String)
        json_string = var.toString().toUtf8();
    else
        json_string = QJsonDocument::fromVariant(var).toJson(QJsonDocument::Compact);

    jbson::document doc;
    try {
        jbson::json_reader reader;
        reader.parse(json_string);
        doc = std::move(reader);
    } catch(...) {
        std::clog << boost::current_exception_diagnostic_information();
        std::clog << "Using empty query" << std::endl;
        doc = "{}"_json_doc;
    }

    return std::move(doc);
}

static QVariant to_qvariant(const jbson::document& doc) {
    QByteArray json;
    try {
        jbson::write_json(doc, std::back_inserter(json));
    } catch(...) {
        return QVariantMap{};
    }

    auto qdoc = QJsonDocument::fromJson(json);
    return qdoc.toVariant();
}

static QVariant to_qvariant(const std::vector<std::tuple<std::string, std::string>>& paths) {
    QVariantMap ret;
    for(auto&& tup : paths)
        ret.insert(QString::fromStdString(std::get<0>(tup)), QString::fromStdString(std::get<1>(tup)));
    return ret;
}

struct FilterPane::impl {
    explicit impl(FilterPane* parent);

    bool on_selection_changed(const QItemSelection& selected, const QItemSelection& deselected);

    void generate_query(bool unknown = false);
    void refresh();

    std::vector<jbson::document> execute_query(const jbson::document&);

    FilterPane* m_parent{nullptr};
    JsonDocModel m_model;
    SelectionModel m_selection_model;

    std::vector<jbson::document_set> m_selection_values;
    std::vector<std::tuple<std::string, std::string>> m_generator_paths;

    std::function<jbson::document(decltype(m_selection_values))> m_query_generator;
    QJSValue m_qml_query_generator;
    jbson::document m_generated_query;

    jbson::document m_unknown_query;

    jbson::document m_query;
    QVariantMap m_paths;
    QVariantList m_sort_fields;

    FilterPane* m_depends{nullptr};
    QString m_depends_path;
    QVariantList m_depend_selection;

    LibraryManager* m_libman;
    boost::optional<ejdb::db> m_db;
    boost::optional<ejdb::collection> m_coll;

    std::atomic<bool> m_lib_scanning{true};

    Logger::Logger logject{logging::keywords::channel = "FilterPane"};
};

FilterPane::impl::impl(FilterPane* parent) : m_parent(parent), m_model(), m_selection_model(&m_model) {
    m_libman = LibraryManager::instance();
    assert(m_libman);
    bool r = QObject::connect(m_libman, &LibraryManager::scanStarted, m_parent, [this]() {
                                                                                    TRACE_LOG(logject)
                                                                                        << "Scan started";
                                                                                    m_lib_scanning.store(true);
                                                                                },
                              Qt::QueuedConnection);
    assert(r);
    r = QObject::connect(m_libman, &LibraryManager::scanEnded, m_parent, [this]() {
                                                                             TRACE_LOG(logject) << "Scan ended";
                                                                             m_lib_scanning.store(false);
                                                                             refresh();
                                                                         },
                         Qt::QueuedConnection);
    assert(r);
    m_lib_scanning.store(m_libman->getLibraryManager() && m_libman->getLibraryManager()->scanning());

    QObject::connect(m_parent, &QObject::objectNameChanged, m_parent, [this](QString name) {
        if(name.isEmpty())
            logject = Logger::Logger{logging::keywords::channel = "FilterPane"};
        else
            logject = Logger::Logger{logging::keywords::channel = ("FilterPane<"s + name.toStdString() + ">")};
    });
}

bool FilterPane::impl::on_selection_changed(const QItemSelection& selected, const QItemSelection& deselected) {
    TRACE_LOG(logject) << "selection changing; selected size: " << selected.size()
                       << "; deselected size: " << deselected.size();
    bool unknowns = false;
    for(auto&& range : selected) {
        for(auto&& i : range.indexes()) {
            try {
                auto str = i.data(JsonDocModel::DocumentStringRole);
                TRACE_LOG(logject) << "doc at index " << i.row() << ": " << qPrintable(str.toString());
                auto res = Library::apply_named_paths(to_document(str), m_generator_paths);
                if(res.empty()) {
                    TRACE_LOG(logject) << "Result of path expression empty; assuming an \"unknown\" selected.";
                    unknowns = true;
                } else
                    m_selection_values.emplace_back(res);
            } catch(...) {
                assert(false);
                continue;
            }
        }
    }
    for(auto&& range : deselected) {
        for(auto&& i : range.indexes()) {
            try {
                auto res = Library::apply_named_paths(to_document(i.data(JsonDocModel::DocumentStringRole)),
                                                      m_generator_paths);

                m_selection_values.erase(boost::range::remove(m_selection_values, res), m_selection_values.end());
            } catch(...) {
                assert(false);
                continue;
            }
        }
    }
    TRACE_LOG(logject) << "selection changed; size: " << m_selection_values.size();
    return unknowns;
}

void FilterPane::impl::generate_query(const bool unknown) {
    jbson::document inner;

    if(!m_selection_values.empty() && m_query_generator) {
        inner = m_query_generator(m_selection_values);
    }

    if(unknown && boost::distance(m_unknown_query) > 0) {
        if(boost::distance(inner) > 0)
            inner = jbson::document(jbson::builder("$or", jbson::array_builder(std::move(inner))(m_unknown_query)));
        else
            inner = m_unknown_query;
    }

    if(boost::distance(m_query) > 0) {
        if(boost::distance(inner) > 0)
            m_generated_query =
                jbson::document(jbson::builder("$and", jbson::array_builder(m_query)(std::move(inner))));
        else
            m_generated_query = m_query;
    } else
        m_generated_query = std::move(inner);

#ifndef NDEBUG
    std::string json_string;
    jbson::write_json(m_generated_query, std::back_inserter(json_string));
    TRACE_LOG(logject) << "generated query json_string: " << json_string;
#endif
    Q_EMIT m_parent->queryGenerated(to_qvariant(m_generated_query));
}

void FilterPane::impl::refresh() {
    m_selection_model.clear();

    std::vector<jbson::document> docs;
    if(m_paths.empty())
        docs = execute_query(m_query);
    else
        try {
            // prepare named paths in stl format
            std::vector<std::tuple<std::string, std::string>> named_paths;
            for(auto it = m_paths.begin(); it != m_paths.end(); ++it) {
                assert(it->isValid());
                assert(it->type() == QVariant::String);
                named_paths.emplace_back(it.key().toStdString(), it->toString().toStdString());
            }

            // run query and transform using named jsonpaths
            TRACE_LOG(logject) << "transforming query results with named JSONPaths";
            auto ds = Library::apply_named_paths(execute_query(m_query), named_paths);

            // prepare sort fields
            std::vector<QByteArray> sort_fields;
            for(auto&& sf : m_sort_fields)
                sort_fields.emplace_back(sf.toString().toUtf8());
            // sort using fields with varying priorities
            TRACE_LOG(logject) << "sorting query results";
            sort_by_criteria(ds, boost::adaptors::transform(
                                     sort_fields, [](auto&& bs) { return boost::string_ref(bs, bs.size()); }));
            // put all unique docs in a vector
            std::transform(ds.begin(), std::unique(ds.begin(), ds.end()), std::back_inserter(docs),
                           [](auto&& d) { return jbson::document{d}; });
        } catch(...) {
            ERROR_LOG(logject) << "refresh(): " << boost::current_exception_diagnostic_information();
        }

    TRACE_LOG(logject) << "setting docs to processed query results";
    m_model.setDocs(std::move(docs));
    TRACE_LOG(logject) << "set docs to processed query results";
}

std::vector<jbson::document> FilterPane::impl::execute_query(const jbson::document& doc) {
    if(m_db) {
        try {
            auto q = m_db->create_query(doc.data());
            if(!q)
                return {};

            std::vector<jbson::document> ret;
            for(auto&& data : m_coll->execute_query(q))
                ret.emplace_back(std::move(data));
            return ret;
        } catch(...) {
            return {};
        }
    }

    assert(m_libman);
    if(m_libman && !m_lib_scanning.load())
        return m_libman->getLibraryManager()->query(doc);

    return {};
}

FilterPane::FilterPane(ejdb::db db, ejdb::collection coll, QObject* parent) : FilterPane(parent) {
    pimpl->m_db = db;
    pimpl->m_coll = coll;
}

FilterPane::FilterPane(QObject* parent) : QObject(parent), pimpl(new impl(this)) {
    connect(this, &FilterPane::queryChanged, [this](const QVariant& qry) {
        pimpl->generate_query();
        pimpl->refresh();
    });
    connect(this, &FilterPane::pathsChanged, [this](auto&&) { pimpl->refresh(); });
    connect(this, &FilterPane::dependsOnChanged, [this](FilterPane* dep) {
        if(dep == nullptr)
            return;

        setQuery(dep->generatedQuery());
        connect(dep, &FilterPane::queryGenerated, [this](const QVariant& qry) { setQuery(qry); });
    });

    connect(selectionModel(), &SelectionModel::selectionChanged,
            [this](auto&& s, auto&& d) { pimpl->generate_query(pimpl->on_selection_changed(s, d)); });
    connect(this, &FilterPane::generatorPathsChanged, [this](auto&&) {
        pimpl->m_selection_values.clear();
        if(selectionModel()->hasSelection())
            pimpl->generate_query(pimpl->on_selection_changed(selectionModel()->selection(), {}));
    });
}

FilterPane::~FilterPane() {}

QAbstractItemModel* FilterPane::model() const { return &pimpl->m_model; }

QVariant FilterPane::unknownQuery() const { return to_qvariant(pimpl->m_unknown_query); }

void FilterPane::setUnknownQuery(QVariant uq) {
    pimpl->m_unknown_query = to_document(uq);
    Q_EMIT unknownQueryChanged(uq);
}

QJSValue FilterPane::queryGenerator() const { return pimpl->m_qml_query_generator; }

struct toJSValue {
    QJSValue operator()(boost::string_ref, jbson::element_type, boost::string_ref str) const {
        return QJSValue{QString::fromLocal8Bit(str.data(), str.size())};
    }
    QJSValue operator()(boost::string_ref, jbson::element_type, bool val) const {
        return QJSValue{val};
    }
    QJSValue operator()(boost::string_ref, jbson::element_type, int32_t val) const {
        return QJSValue{val};
    }
    QJSValue operator()(boost::string_ref, jbson::element_type, double val) const {
        return QJSValue{val};
    }

    template <typename T> QJSValue operator()(boost::string_ref, jbson::element_type, T&&) const {
        return QJSValue{};
    }

    QJSValue operator()(boost::string_ref, jbson::element_type) const {
        return QJSValue{};
    }
};

template <typename SelectionT>
static jbson::document qml_generator(SelectionT&& selection, QJSValue fun, QJSEngine* engine) {
    assert(fun.isCallable());
    if(!fun.isCallable())
        return {};
    assert(engine != nullptr);
    if(!engine)
        return {};

    QJSValueList call_args;
    for(auto&& set : selection) {
        auto obj = engine->newObject();
        for(auto&& elem : set)
            obj.setProperty(QString::fromLocal8Bit(elem.name().data(), elem.name().size()), elem.visit(toJSValue{}));
        call_args << obj;
    }
    auto ret = fun.call(call_args);
    assert(!ret.isError());
    QVariant variant = ret.toVariant();
    assert(variant.isValid() && !variant.isNull());

    if(variant.type() == QVariant::String || variant.type() == QVariant::Map)
        return to_document(variant);

    std::clog << "qml_generator error: unknown generated value type; " << variant.typeName() << std::endl;
    assert(false);
    return {};
}

void FilterPane::setQueryGenerator(QJSValue val) {
    pimpl->m_qml_query_generator = val;
    pimpl->m_query_generator = [this, &fun = pimpl->m_qml_query_generator](const auto&& selection) {
        return qml_generator(selection, fun, qmlEngine(this));
    };
    Q_EMIT queryGeneratorChanged(val);
}

void FilterPane::setQueryGenerator(std::function<jbson::document(const std::vector<jbson::document_set>&)> fun) {
    assert(fun);
    pimpl->m_query_generator = std::move(fun);
    pimpl->generate_query();
}

QVariant FilterPane::generatorPaths() const { return to_qvariant(pimpl->m_generator_paths); }

void FilterPane::setGeneratorPaths(QVariant p) {
    if(p.type() == QVariant::Map) {
        pimpl->m_generator_paths.clear();
        auto map = p.value<QVariantMap>();
        TRACE_LOG(pimpl->logject) << "setGeneratorPaths(); map.size: " << map.size();
        for(auto it = map.begin(); it != map.end(); it++) {
            TRACE_LOG(pimpl->logject) << "setGeneratorPaths(); [\"" << qPrintable(it.key()) << "\": \""
                                      << it.value().toString().toStdString() << "\"]";
            pimpl->m_generator_paths.emplace_back(it.key().toStdString(), it.value().toString().toStdString());
        }
        Q_EMIT generatorPathsChanged(p);
        return;
    } else if(p.type() == QVariant::String) {
        TRACE_LOG(pimpl->logject) << "setGeneratorPaths(); " << qPrintable(p.toString());
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(p.toString().toUtf8(), &err);
        if(err.error)
            ERROR_LOG(pimpl->logject) << "setGeneratorPaths(); invalid JSON string";
        else
            setGeneratorPaths(doc.toVariant());
    }
}

QVariant FilterPane::generatedQuery() const { return to_qvariant(pimpl->m_generated_query); }

QVariant FilterPane::query() const { return to_qvariant(pimpl->m_query); }

void FilterPane::setQuery(QVariant q) {
    pimpl->m_query = to_document(q);
    Q_EMIT queryChanged(q);
}

QVariant FilterPane::paths() const { return pimpl->m_paths; }

void FilterPane::setPaths(QVariant p) {
    if(p == pimpl->m_paths)
        return;
    if(p.type() == QVariant::Map) {
        pimpl->m_paths = p.value<QVariantMap>();
        Q_EMIT pathsChanged(p);
        return;
    } else if(p.type() == QVariant::String)
        setPaths(QJsonDocument::fromJson(p.toString().toUtf8()).toVariant());
}

QVariantList FilterPane::sortFields() const { return pimpl->m_sort_fields; }

void FilterPane::setSortFields(QVariantList list) {
    pimpl->m_sort_fields = list;
    Q_EMIT sortFieldsChanged(list);
}

FilterPane* FilterPane::dependsOn() const { return pimpl->m_depends; }

void FilterPane::setDependsOn(FilterPane* d) {
    if(d == pimpl->m_depends)
        return;
    pimpl->m_depends = d;
    Q_EMIT dependsOnChanged(d);
}

SelectionModel* FilterPane::selectionModel() const { return &pimpl->m_selection_model; }

} // namespace Melosic
