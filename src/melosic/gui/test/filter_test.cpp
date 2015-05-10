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

#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QVariant>

#include <boost/range/algorithm/transform.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <ejpp/ejdb.hpp>
#include <jbson/json_reader.hpp>
#include <jbson/json_writer.hpp>
#include <jbson/builder.hpp>
using namespace jbson::literal;

#include <melosic/gui/filterpane.hpp>
#include <melosic/melin/logging.hpp>
using namespace Melosic;

template <typename RangeT>
static std::vector<jbson::element>
to_array(RangeT&& range, std::enable_if_t<jbson::detail::is_range_of_value<RangeT,
         boost::mpl::quote1<jbson::detail::is_element>>::value>* = nullptr) {
    std::vector<jbson::element> arr;
    size_t i{0};
    boost::transform(range, std::back_inserter(arr), [&i](auto&& val) {
        jbson::element el{val};
        el.name(std::to_string(i++));
        return el;
    });
    assert(boost::distance(arr) == boost::distance(range));
    return std::move(arr);
}

template <typename RangeT>
static std::vector<jbson::element>
to_array(RangeT&& range, std::enable_if_t<jbson::detail::is_range_of_value<
         RangeT, boost::mpl::bind<boost::mpl::quote2<jbson::detail::is_range_of_value>, boost::mpl::arg<1>,
         boost::mpl::quote1<jbson::detail::is_element>>>::value>* = nullptr) {
    std::vector<jbson::element> arr;
    size_t i{0};
    boost::transform(range | boost::adaptors::filtered([](auto&& val) { return boost::distance(val) >= 1; }),
                     std::back_inserter(arr), [&i](auto&& val) {
        jbson::element el{*val.begin()};
        el.name(std::to_string(i++));
        return el;
    });
    return std::move(arr);
}

struct FilterTest : QObject {
    Q_OBJECT

    std::unique_ptr<FilterPane> pane1;
    std::unique_ptr<FilterPane> pane2;
    std::unique_ptr<FilterPane> pane3;

    ejdb::db db;
    ejdb::collection coll;
    std::error_code ec;

private Q_SLOTS:
    // global
    void initTestCase() {
        Logger::init();
        db.open(QDir::tempPath().toStdString()+ "/testdb", ejdb::db_mode::read|
                                                ejdb::db_mode::write|
                                                ejdb::db_mode::create|
                                                ejdb::db_mode::truncate|
                                                ejdb::db_mode::trans_sync, ec);
        QVERIFY(!ec);
        QVERIFY(db.is_open());

        coll = db.create_collection("test", ec);
        QVERIFY(!ec);
        QVERIFY(static_cast<bool>(coll));

        // create data
        coll.save_document(R"({"name": "Bill", "age": 32, "city": "London"})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Alice", "age": 32, "city": "Bristol"})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Steve", "age": 21, "city": "Birmingham"})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Fred", "age": 46, "city": "Manchester"})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Jim", "age": 46, "city": "London"})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Jane", "age": 32, "city": "Liverpool"})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Bob", "age": 21, "city": "Manchester"})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Dave", "age": 32, "city": "London"})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Alice", "age": 21, "city": "Manchester"})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Bill", "age": 25, "city": "London"})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Bob", "age": 45})"_json_doc.data(), ec);
        coll.save_document(R"({"name": "Bill", "city": "London"})"_json_doc.data(), ec);
        coll.save_document(R"({"age": 42, "city": "Bristol"})"_json_doc.data(), ec);

        coll.sync(ec);
        QVERIFY(!ec);

        auto all = coll.get_all();
        QVERIFY(!all.empty());

        coll = db.get_collection("test", ec);
        QVERIFY(!ec);
        QVERIFY(static_cast<bool>(coll));

        all = coll.get_all();
        QVERIFY(!all.empty());

        QVERIFY(db.sync(ec));
        QVERIFY(!ec);

        QVERIFY(db.close(ec));
        QVERIFY(!ec);
        QVERIFY(!db.is_open());

        QVERIFY(db.open(QDir::tempPath().toStdString()+ "/testdb", ejdb::db_mode::read|
                        ejdb::db_mode::write|
                        ejdb::db_mode::create|
                        ejdb::db_mode::trans_sync, ec));
        QVERIFY(!ec);
        QVERIFY(db.is_open());

        coll = db.get_collection("test", ec);
        QVERIFY(!ec);
        QVERIFY(static_cast<bool>(coll));

        all = coll.get_all();
        QVERIFY(!all.empty());
    }

    void cleanupTestCase() {
        QVERIFY(!ec);
        db.close(ec);
        QVERIFY(!ec);
    }

    // every test
    void init() {
        QVERIFY(!ec);

        pane1 = std::make_unique<FilterPane>(db, coll);
        pane1->setObjectName(QStringLiteral("pane1"));
        pane1->setPaths(QStringLiteral(R"({"city": "$.city"})"));
        pane1->setQuery(QStringLiteral("{}"));

        pane1->setQueryGenerator([](const std::vector<jbson::document_set>& selection) -> jbson::document {
            std::clog << "generating query from pane1 selection of " << selection.size() << " elements\n";
            auto in_arr = to_array(selection);

            jbson::builder qry;
            qry = jbson::builder("city", jbson::element_type::document_element, jbson::builder
                                 ("$in", jbson::element_type::array_element, in_arr)
                                );
            if(selection.size() != in_arr.size()) {
                auto unknown = jbson::builder("city", jbson::builder("$exists", false));
                if(in_arr.empty())
                    qry = std::move(unknown);
                else
                    qry = jbson::builder("$or", jbson::array_builder(qry)(unknown));
            }
            return jbson::document(std::move(qry));
        });
        pane1->setGeneratorPaths(QStringLiteral(R"({ "city": "$.city" })"));
        pane1->setSelectionDelay(0);

        QCOMPARE(pane1->model()->rowCount(), 6);

        pane2 = std::make_unique<FilterPane>(db, coll);
        pane2->setObjectName(QStringLiteral("pane2"));
        pane2->setPaths(QStringLiteral(R"({"age": "$.age"})"));
        pane2->setDependsOn(pane1.get());

        pane2->setQueryGenerator([](const std::vector<jbson::document_set>& selection) -> jbson::document {
            std::clog << "generating query from pane1 selection of " << selection.size() << " elements\n";
            auto in_arr = to_array(selection);

            jbson::builder qry;
            qry = jbson::builder("age", jbson::element_type::document_element, jbson::builder
                                 ("$in", jbson::element_type::array_element, in_arr)
                                );
            if(selection.size() != in_arr.size()) {
                auto unknown = jbson::builder("age", jbson::builder("$exists", false));
                if(in_arr.empty())
                    qry = std::move(unknown);
                else
                    qry = jbson::builder("$or", jbson::array_builder(qry)(unknown));
            }
            return jbson::document(std::move(qry));
        });
        pane2->setGeneratorPaths(QStringLiteral(R"({ "age": "$.age"})"));
        pane2->setSelectionDelay(0);

        QCOMPARE(pane2->model()->rowCount(), 7);

        pane3 = std::make_unique<FilterPane>(db, coll);
        pane3->setObjectName(QStringLiteral("pane3"));
        pane3->setPaths(QStringLiteral(R"({"name": "$.name"})"));
        pane3->setDependsOn(pane2.get());

        QCOMPARE(pane3->model()->rowCount(), 9);

        std::clog << "finish init()" << std::endl;
    }

    void cleanup() {
        QVERIFY(!ec);
        pane3.reset();
        pane2.reset();
        pane1.reset();
    }

    // tests
    void queryTest1() {
        auto query = db.create_query("{}"_json_doc.data(), ec);
        QVERIFY(!ec);
        auto results = coll.execute_query(query);
        QVERIFY(!results.empty());

        QCOMPARE(results.size(), static_cast<decltype(results.size())>(13));
    }
    void queryTest2() {
        auto query = db.create_query("{}"_json_doc.data(), ec);
        QVERIFY(!ec);

        std::vector<jbson::document> docs;
        for(auto&& data : coll.execute_query(query))
            docs.emplace_back(std::move(data));
        auto results = Melosic::Library::apply_named_paths(docs, {{"age", "$.age"}});
        QVERIFY(!results.empty());
        boost::sort(results);

        docs.clear();
        std::transform(results.begin(), std::unique(results.begin(), results.end()),
                       std::back_inserter(docs), [](auto&& d) { return jbson::document{d}; });

        QCOMPARE(docs.size(), static_cast<decltype(results.size())>(7));
    }

    void filterTest1() {
        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 7);
        QCOMPARE(pane3->model()->rowCount(), 9);

        QSignalSpy spy1{pane1.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy1.isValid());
        QSignalSpy spy2{pane2.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy2.isValid());
        QSignalSpy spy3{pane2.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy3.isValid());

        auto idxs = pane1->model()->match(pane1->model()->index(0, 0),
                                          JsonDocModel::DocumentStringRole,
                                          "London", 1,
                                          Qt::MatchContains);
        QCOMPARE(idxs.size(), 1);

        pane1->selectionModel()->select(idxs[0], QItemSelectionModel::ClearAndSelect);
        QCOMPARE(spy1.count(), 1);
        QCOMPARE(spy2.count(), 1);
        QCOMPARE(spy3.count(), 1);

        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 4);
        QCOMPARE(pane3->model()->rowCount(), 3);

        QVariant data;

        data = pane2->model()->index(0,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral("{  }"));

        data = pane2->model()->index(1,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "age" : 25 })"));

        data = pane2->model()->index(2,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "age" : 32 })"));

        data = pane2->model()->index(3,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "age" : 46 })"));

        data = pane3->model()->index(0,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Bill" })"));

        data = pane3->model()->index(1,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Dave" })"));

        data = pane3->model()->index(2,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Jim" })"));

        pane1->selectionModel()->clear();
        QCOMPARE(spy1.count(), 2);
        QCOMPARE(spy2.count(), 2);
        QCOMPARE(spy3.count(), 2);

        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 7);
        QCOMPARE(pane3->model()->rowCount(), 9);
    }

    void filterTest2() {
        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 7);
        QCOMPARE(pane3->model()->rowCount(), 9);

        QSignalSpy spy1{pane1.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy1.isValid());
        QSignalSpy spy2{pane2.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy2.isValid());
        QSignalSpy spy3{pane2.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy3.isValid());

        QItemSelection selection;

        auto idxs = pane1->model()->match(pane1->model()->index(0, 0),
                                          JsonDocModel::DocumentStringRole,
                                          "Manchester", 1,
                                          Qt::MatchContains);
        QCOMPARE(idxs.size(), 1);
        selection.push_back(QItemSelectionRange(idxs[0]));

        idxs = pane1->model()->match(pane1->model()->index(0, 0),
                                          JsonDocModel::DocumentStringRole,
                                          "Bristol", 1,
                                          Qt::MatchContains);
        QCOMPARE(idxs.size(), 1);
        selection.push_back(QItemSelectionRange(idxs[0]));

        pane1->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
        QCOMPARE(spy1.count(), 1);
        QCOMPARE(spy2.count(), 1);
        QCOMPARE(spy3.count(), 1);

        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 4);
        QCOMPARE(pane3->model()->rowCount(), 4);

        QVariant data;

        data = pane2->model()->index(0,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "age" : 21 })"));

        data = pane2->model()->index(1,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "age" : 32 })"));

        data = pane2->model()->index(2,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "age" : 42 })"));

        data = pane2->model()->index(3,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "age" : 46 })"));

        data = pane3->model()->index(0,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({  })"));

        data = pane3->model()->index(1,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Alice" })"));

        data = pane3->model()->index(2,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Bob" })"));

        data = pane3->model()->index(3,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Fred" })"));

        pane1->selectionModel()->clear();
        QCOMPARE(spy1.count(), 2);
        QCOMPARE(spy2.count(), 2);
        QCOMPARE(spy3.count(), 2);

        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 7);
        QCOMPARE(pane3->model()->rowCount(), 9);
    }

    void filterTest3() {
        QCOMPARE(pane1->model()->rowCount(), 6);
        QSignalSpy spy1{pane1.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy1.isValid());
        QSignalSpy spy2{pane2.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy2.isValid());
        QSignalSpy spy3{pane2.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy3.isValid());

        auto idxs = pane2->model()->match(pane2->model()->index(0, 0),
                                          JsonDocModel::DocumentStringRole,
                                          "32", 1,
                                          Qt::MatchContains);
        QCOMPARE(idxs.size(), 1);

        pane2->selectionModel()->select(idxs[0], QItemSelectionModel::ClearAndSelect);
        QVERIFY(pane2->selectionModel()->isSelected(idxs[0]));

        QCOMPARE(spy1.count(), 0);
        QCOMPARE(spy2.count(), 1);
        QCOMPARE(spy3.count(), 1);

        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 7);
        QCOMPARE(pane3->model()->rowCount(), 4);

        QVariant data;

        data = pane3->model()->index(0,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Alice" })"));

        data = pane3->model()->index(1,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Bill" })"));

        data = pane3->model()->index(2,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Dave" })"));

        data = pane3->model()->index(3,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Jane" })"));

        pane2->selectionModel()->clear();
        QCOMPARE(spy1.count(), 0);
        QCOMPARE(spy2.count(), 2);
        QCOMPARE(spy3.count(), 2);

        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 7);
        QCOMPARE(pane3->model()->rowCount(), 9);
    }

    void filterTest4() {
        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 7);
        QCOMPARE(pane3->model()->rowCount(), 9);

        QSignalSpy spy1{pane1.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy1.isValid());
        QSignalSpy spy2{pane2.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy2.isValid());
        QSignalSpy spy3{pane2.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy3.isValid());

        auto idxs = pane1->model()->match(pane1->model()->index(0, 0),
                                          JsonDocModel::DocumentStringRole,
                                          "{  }", 1,
                                          Qt::MatchExactly);
        QCOMPARE(idxs.size(), 1);

        pane1->selectionModel()->select(idxs[0], QItemSelectionModel::ClearAndSelect);
        QCOMPARE(spy1.count(), 1);
        QCOMPARE(spy2.count(), 1);
        QCOMPARE(spy3.count(), 1);

        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 1);
        QCOMPARE(pane3->model()->rowCount(), 1);

        QVariant data;

        data = pane2->model()->index(0,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "age" : 45 })"));

        data = pane3->model()->index(0,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Bob" })"));

        pane1->selectionModel()->clear();
        QCOMPARE(spy1.count(), 2);
        QCOMPARE(spy2.count(), 2);
        QCOMPARE(spy3.count(), 2);

        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 7);
        QCOMPARE(pane3->model()->rowCount(), 9);
    }

    void filterTest5() {
        QCOMPARE(pane1->model()->rowCount(), 6);
        QSignalSpy spy1{pane1.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy1.isValid());
        QSignalSpy spy2{pane2.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy2.isValid());
        QSignalSpy spy3{pane2.get(), SIGNAL(queryGenerated(QVariant))};
        QVERIFY(spy3.isValid());

        auto idxs = pane2->model()->match(pane2->model()->index(0, 0),
                                          JsonDocModel::DocumentStringRole,
                                          "{  }", 1,
                                          Qt::MatchExactly);
        QCOMPARE(idxs.size(), 1);

        pane2->selectionModel()->select(idxs[0], QItemSelectionModel::ClearAndSelect);
        QVERIFY(pane2->selectionModel()->isSelected(idxs[0]));

        QCOMPARE(spy1.count(), 0);
        QCOMPARE(spy2.count(), 1);
        QCOMPARE(spy3.count(), 1);

        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 7);
        QCOMPARE(pane3->model()->rowCount(), 1);

        QVariant data;

        data = pane3->model()->index(0,0).data(JsonDocModel::DocumentStringRole);
        QVERIFY(data.type() == QVariant::String);
        QCOMPARE(data.toString(), QStringLiteral(R"({ "name" : "Bill" })"));

        pane2->selectionModel()->clear();
        QCOMPARE(spy1.count(), 0);
        QCOMPARE(spy2.count(), 2);
        QCOMPARE(spy3.count(), 2);

        QCOMPARE(pane1->model()->rowCount(), 6);
        QCOMPARE(pane2->model()->rowCount(), 7);
        QCOMPARE(pane3->model()->rowCount(), 9);
    }
};

QTEST_MAIN(FilterTest)
#include "filter_test.moc"
