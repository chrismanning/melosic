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

#include <vector>
#include <set>

#include <jbson/document.hpp>
#include <jbson/json_reader.hpp>
#include <jbson/json_writer.hpp>
using namespace jbson;

namespace std {

template <typename T>
ostream& operator<<(ostream& os, const vector<T>& ds) {
    auto end = ds.end();
    os << '[';
    for(auto i = ds.begin(); i != end;) {
        os << "\n\t" << *i;
        if(++i != end)
            os << ',';
    }
    os << ']';
    return os;
}

} // std

namespace jbson {

template <typename C>
std::ostream& operator<<(std::ostream& os, const basic_document_set<C>& ds) {
    std::string str;
    write_json(basic_document<C>(ds), std::back_inserter(str));
    os << str;
    return os;
}

template <typename C1, typename C2>
std::ostream& operator<<(std::ostream& os, const basic_document<C1,C2>& ds) {
    std::string str;
    write_json(ds, std::back_inserter(str));
    os << str;
    return os;
}

} // jbson

#include <catch.hpp>

template <typename AssocT, typename ValueT>
auto find_optional(AssocT&& rng, ValueT&& val) {
    boost::optional<typename boost::range_value<std::decay_t<AssocT>>::type> ret;
    auto it = rng.find(val);
    if(it != std::end(rng))
        return ret = *it;
    return ret = boost::none;
}

template <typename AssocRangeT, typename StringRangeT>
static inline void sort_by_criteria_impl(AssocRangeT& rng, const StringRangeT& sort_fields) {
    boost::range::sort(rng, [&](auto&& a, auto&& b) {
        for(const auto& sf: sort_fields) {
            auto val1 = find_optional(a, sf);
            auto val2 = find_optional(b, sf);
            if(val1 == val2)
                continue;
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

TEST_CASE("SortTest1") {
    std::vector<document_set> docs;
    docs.push_back(R"({"title":"some title","tracknumber":"2"})"_json_set);
    docs.push_back(R"({"title":"some title","tracknumber":"9"})"_json_set);
    docs.push_back(R"({"title":"some title II","tracknumber":"2"})"_json_set);
    decltype(docs) ex_docs;
    ex_docs.push_back(R"({"title":"some title","tracknumber":"2"})"_json_set);
    ex_docs.push_back(R"({"title":"some title II","tracknumber":"2"})"_json_set);
    ex_docs.push_back(R"({"title":"some title","tracknumber":"9"})"_json_set);
    REQUIRE(3u == docs.size());

    sort_by_criteria(docs, {"tracknumber", "title"});
    CHECK(ex_docs == docs);
}
