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

#include <cassert>
#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>
#include <boost/format.hpp>

#include <mutex>
#include <shared_mutex>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
using mutex = boost::shared_mutex;
using shared_lock = boost::shared_lock<mutex>;
using unique_lock = boost::unique_lock<mutex>;
using upgrade_lock = boost::upgrade_lock<mutex>;

#include <melosic/melin/logging.hpp>
#include <melosic/common/scope_unlock_exit_lock.hpp>

#include "categoryproxymodel.hpp"
#include "category.hpp"

namespace Melosic {

Logger::Logger logject{logging::keywords::channel = "CategoryProxyModel"};

struct CategoryProxyModel::impl {
    explicit impl(CategoryProxyModel& parent) : parent(parent) {}

    std::vector<std::shared_ptr<Block>> block_index;
    mutex mu;

    void updateBlock(int idx, std::shared_ptr<Block>& block, upgrade_lock&);
    QString indexCategory(const QModelIndex& index);

    QString indexCategory(int index) {
        return indexCategory(parent.index(index, 0));
    }

    void onRowsInserted(const QModelIndex&, int start, int end, upgrade_lock& l);
    void onRowsMoved(const QModelIndex&, int sourceStart, int sourceEnd, const QModelIndex&, int destinationRow, upgrade_lock& l);
    void onRowsAboutToBeMoved(const QModelIndex&, int sourceStart, int sourceEnd,
                              const QModelIndex&, int destinationRow, upgrade_lock& l);
    void onRowsRemoved(const QModelIndex&, int start, int end, upgrade_lock& l);
    void onRowsAboutToBeRemoved(const QModelIndex&, int start, int end, upgrade_lock& l);

    void onDataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&, upgrade_lock& l);

    void checkForConsistency(upgrade_lock&);

    Category* m_category = nullptr;
    CategoryProxyModel& parent;
};

CategoryProxyModel::CategoryProxyModel(QObject* parent)
    :
      QIdentityProxyModel(parent),
      pimpl(new impl(*this))
{
    connect(this, &CategoryProxyModel::rowsInserted, this, &CategoryProxyModel::onRowsInserted);
    connect(this, &CategoryProxyModel::rowsMoved, this, &CategoryProxyModel::onRowsMoved);
    connect(this, &CategoryProxyModel::rowsAboutToBeMoved, this, &CategoryProxyModel::onRowsAboutToBeMoved);
    connect(this, &CategoryProxyModel::rowsRemoved, this, &CategoryProxyModel::onRowsRemoved);
    connect(this, &CategoryProxyModel::rowsAboutToBeRemoved, this, &CategoryProxyModel::onRowsAboutToBeRemoved);
    connect(this, &CategoryProxyModel::dataChanged, this, &CategoryProxyModel::onDataChanged);
    connect(this, &CategoryProxyModel::categoryChanged, [this] (Category* c) {
        TRACE_LOG(logject) << "Category changed";
        assert(c);
        assert(c->model());
        if(rowCount() > 0)
            onDataChanged(index(0,0), index(sourceModel()->rowCount() - 1, 0), {});
    });
}

CategoryProxyModel::~CategoryProxyModel() {}

QString CategoryProxyModel::indexCategory(const QModelIndex& index) const {
    upgrade_lock l(pimpl->mu);
    return pimpl->indexCategory(index);
}

Category* CategoryProxyModel::category() const {
    shared_lock l(pimpl->mu);
    return pimpl->m_category;
}

void CategoryProxyModel::setCategory(Category* c) {
    unique_lock l(pimpl->mu);
    pimpl->m_category = c;
    if(c == nullptr)
        return;
    if(!c->model())
        c->setModel(this);
    else
        assert(c->model() == this);
    l.unlock();
    Q_EMIT categoryChanged(c);
}

void CategoryProxyModel::onRowsInserted(const QModelIndex& p, int start, int end) {
    upgrade_lock l(pimpl->mu);
    pimpl->onRowsInserted(p, start, end, l);
#ifndef NDEBUG
//    pimpl->checkForConsistency(l);
#endif
}

void CategoryProxyModel::onRowsMoved(const QModelIndex& sp,
                                     int sourceStart,
                                     int sourceEnd,
                                     const QModelIndex& dp,
                                     int destinationRow)
{
    upgrade_lock l(pimpl->mu);
    pimpl->onRowsMoved(sp, sourceStart, sourceEnd, dp, destinationRow, l);
#ifndef NDEBUG
    pimpl->checkForConsistency(l);
#endif
}

void CategoryProxyModel::onRowsAboutToBeMoved(const QModelIndex& sp,
                                              int sourceStart,
                                              int sourceEnd,
                                              const QModelIndex& dp,
                                              int destinationRow)
{
    upgrade_lock l(pimpl->mu);
    pimpl->onRowsAboutToBeMoved(sp, sourceStart, sourceEnd, dp, destinationRow, l);
#ifndef NDEBUG
    pimpl->checkForConsistency(l);
#endif
}

void CategoryProxyModel::onRowsRemoved(const QModelIndex& p, int start, int end) {
    upgrade_lock l(pimpl->mu);
    pimpl->onRowsRemoved(p, start, end, l);
#ifndef NDEBUG
    pimpl->checkForConsistency(l);
#endif
}

void CategoryProxyModel::onRowsAboutToBeRemoved(const QModelIndex& p, int start, int end) {
    upgrade_lock l(pimpl->mu);
    pimpl->onRowsAboutToBeRemoved(p, start, end, l);
#ifndef NDEBUG
    pimpl->checkForConsistency(l);
#endif
}

void CategoryProxyModel::onDataChanged(const QModelIndex& topleft,
                                       const QModelIndex& bottomright,
                                       const QVector<int>& roles)
{
    upgrade_lock l(pimpl->mu);
    pimpl->onDataChanged(topleft, bottomright, roles, l);
#ifndef NDEBUG
    pimpl->checkForConsistency(l);
#endif
}

void CategoryProxyModel::impl::checkForConsistency(upgrade_lock& l) {
    assert(l);
    assert(block_index.size() == (size_t)parent.rowCount());
    std::shared_ptr<Block> prev_block;
    QString prev_category;
    for(auto i = 0; i < parent.rowCount(); ++i) {
        assert(parent.hasIndex(i, 0));
        const auto category = indexCategory(i);
        const auto block = block_index[i];
        BOOST_ASSERT_MSG(block, (boost::format("index %1% has null block")%i).str().c_str());
        assert(!category.isNull());
        if(category == prev_category) {
            BOOST_ASSERT_MSG(block == prev_block, (boost::format("index %1% has wrong block")%i).str().c_str());
        }
        else {
            BOOST_ASSERT_MSG(block != prev_block, (boost::format("index %1% has wrong block")%i).str().c_str());
            if(prev_block)
                BOOST_ASSERT_MSG(prev_block->firstRow() + prev_block->count() == i,
                                 (boost::format("prev_block should end before current index.\n"
                                               "prev_block: 1st: %1%; count: %2%; i: %3%")
                                 % prev_block->firstRow() % prev_block->count() % i).str().c_str());
        }
        prev_category = category;
        prev_block = block;
    }
}

void CategoryProxyModel::impl::updateBlock(const int idx, std::shared_ptr<Block>& block, upgrade_lock& l) {
    assert(idx >= 0);
    assert(idx < (int)block_index.size());

    TRACE_LOG(logject) << "Updating block for index " << idx;

    const auto category = indexCategory(idx);
    if(category.isNull() | category.isEmpty()) {
        boost::upgrade_to_unique_lock<mutex> ul(l);
        block = nullptr;
        return;
    }

    if(idx > 0) {
        boost::upgrade_to_unique_lock<mutex> ul(l);
        const auto prev_category = indexCategory(idx-1);
        if(prev_category == category) { // same category => same block
            if(block_index[idx-1] && block_index[idx-1] == block) // same block
                return;
            if(block) { // already has own block
                block->setCount(block->count()-1); // decrement block count
                if(block->firstRow() == idx && block->count() > 0) // is start of block
                    block->setFirstIndex(parent.index(idx + 1, 0)); // move start forward
            }

            if(block_index[idx-1]) {
                block = block_index[idx-1]; // set block to previous block
                assert(block->firstRow() + block->count() <= idx);
                block->setCount(block->count()+1); // increase block count
            }
            else {
                block = std::make_shared<Block>();
                block->setFirstIndex(parent.index(idx, 0));
                block->setCount(1);
            }
        }
        else { // different category => different block
            if(block) { // already has own block
                if(block->firstRow() != idx) // idx is erroneously somewhere in the middle
                    block->setFirstIndex(parent.index(idx, 0)); // set as first in block
                const auto first = std::next(block_index.begin(), idx);
                const auto last = std::find_if(first, block_index.end(),
                                               [block] (auto&& a) { return a != block; });
                block->setCount(std::distance(first, last));
            }
            else { // needs block
                block = std::make_shared<Block>();
                block->setFirstIndex(parent.index(idx, 0));
                block->setCount(1);
            }
        }
    }
    else if(idx == 0) {
        boost::upgrade_to_unique_lock<mutex> ul(l);
        if(block) {
            if(block->firstRow() != idx)
                block->setFirstIndex(parent.index(idx, 0));
            const auto first = std::next(block_index.begin(), idx);
            const auto last = std::find_if(first, block_index.end(),
                                           [block] (auto&& a) { return a != block; });
            block->setCount(std::distance(first, last));
        }
        else {
            block = std::make_shared<Block>();
            block->setFirstIndex(parent.index(idx, 0));
            block->setCount(1);
        }
    }

    assert(block);
}

QString CategoryProxyModel::impl::indexCategory(const QModelIndex& index) {
    assert(index.isValid());

    if(!m_category)
        return {};

    QString cat;
    for(Criterion* c : m_category->m_criteria)
        cat += c->result(index);
    return cat;
}

void CategoryProxyModel::impl::onRowsInserted(const QModelIndex&, int first, int last, upgrade_lock &l) {
    if(!m_category)
        return;
    assert(l);
    int trail_ref{0};
    {
        boost::upgrade_to_unique_lock<mutex> ul(l);
        if(first > 0) {
            auto b = block_index[first-1];
            auto b_end = b->firstRow() + b->count();
            if(b_end >= first) {
                for(auto i = first; i < b_end; ++i,++trail_ref) {
                    assert(i < (int)block_index.size());
                    block_index[i] = nullptr;
                }
                b->setCount(first - b->firstRow());
            }
            BOOST_ASSERT_MSG(b->firstRow() + b->count() == first,
                             (boost::format("b block should end before current index.\n"
                                           "b: 1st: %1%; count: %2%; idx: %3%")
                             % b->firstRow() % b->count() % first).str().c_str());
        }
        block_index.insert(std::next(block_index.begin(), first), last-first+1, std::shared_ptr<Block>{nullptr});
    }

    for(auto i = 1; i <= trail_ref && (i+last) < (int)block_index.size(); ++i)
        updateBlock(last+i, block_index[last+i], l);
    if(last+1 < (int)block_index.size() && block_index[last+1])
        assert(block_index[last+1]->firstRow() == last+1);

    TRACE_LOG(logject) << "Rows inserted: " << first << " - " << last;
}

void CategoryProxyModel::impl::onRowsMoved(const QModelIndex&, int sourceStart,
                                           int sourceEnd, const QModelIndex&,
                                           int destinationRow, upgrade_lock& l)
{
    assert(sourceEnd >= sourceStart);
    auto s = sourceEnd - sourceStart;
    auto dest = destinationRow < sourceStart ? destinationRow : destinationRow - (s + 1);

    onDataChanged(this->parent.index(dest, 0), this->parent.index(dest+s, 0), {}, l);
}

void CategoryProxyModel::impl::onRowsAboutToBeMoved(const QModelIndex& parent, int sourceStart, int sourceEnd,
                                                    const QModelIndex&, int, upgrade_lock& l) {
    onRowsAboutToBeRemoved(parent, sourceStart, sourceEnd, l);
}

void CategoryProxyModel::impl::onRowsRemoved(const QModelIndex&, int start, int end, upgrade_lock& l) {
    assert(l);
    if(!m_category || parent.rowCount() == 0) {
        boost::upgrade_to_unique_lock<mutex> ul(l);
        block_index.clear();
        return;
    }
    TRACE_LOG(logject) << "Rows removed: " << start << " - " << end;
    {
        boost::upgrade_to_unique_lock<mutex> ul(l);
        block_index.erase(std::next(block_index.begin(), start), std::next(block_index.begin(), end+1));
    }

    end = start;
    if(--start > (int)block_index.size())
        start = block_index.size()-1;
    if(end >= (int)block_index.size())
        end = block_index.size()-1;

    scope_unlock_exit_lock<upgrade_lock> sl(l);
    Q_EMIT parent.dataChanged(parent.index(start, 0), parent.index(end, 0));
}

void CategoryProxyModel::impl::onRowsAboutToBeRemoved(const QModelIndex&, int start, int end, upgrade_lock& l) {
    assert(l);
    boost::upgrade_to_unique_lock<mutex> ul(l);
    if(!m_category || parent.rowCount() == 0) {
        block_index.clear();
        return;
    }
    TRACE_LOG(logject) << "Rows about to be removed: " << start << " - " << end;

    assert(end < std::distance(block_index.begin(), block_index.end()));
    for(auto i = start; i <= end; ++i) {
        TRACE_LOG(logject) << "Decreasing block count";
        block_index[i]->setCount(block_index[i]->count()-1);
        if(block_index[i]->firstRow() == i && (i+1 < parent.rowCount())) {
            TRACE_LOG(logject) << "Moving block forward 1";
            block_index[i]->setFirstIndex(parent.index(i+1, 0));
        }
    }
}

void CategoryProxyModel::impl::onDataChanged(const QModelIndex& istart,
                                             const QModelIndex& iend,
                                             const QVector<int>&,
                                             upgrade_lock& l)
{
    if(!istart.isValid() || !iend.isValid())
        return;
    assert(l);

    const auto start = istart.row();
    const auto end = iend.row();
    TRACE_LOG(logject) << "data changed; rows " << start << " - " << end;

    for(auto i = start; i <= end; i++) {
        assert(l);
        updateBlock(i, block_index[i], l);
    }

    // merge forward
    auto n = 0;
    assert(block_index.size() == (size_t)parent.rowCount());
    const auto category = indexCategory(end);
    const auto block = block_index[end];
    assert(block);
    {
        boost::upgrade_to_unique_lock<mutex> ul(l);
        for(++n; end+n < (int)block_index.size(); ++n) {
            const auto next_category = indexCategory(end+n);
            if(category != next_category)
                break;
            auto& next_block = block_index[end+n];
            if(next_block == block)
                break;

            TRACE_LOG(logject) << "Merging block forward with category \"" << qPrintable(category) << '"';
            block->setCount(block->count()+1);

            if(!next_block) {
                next_block = block;
                continue;
            }

            next_block->setCount(next_block->count()-1);
            if(next_block->firstRow() == end+n && next_block->count() >= 1)
                next_block->setFirstIndex(parent.index(end+n+1,0));
            next_block = block;
        }
    }

    scope_unlock_exit_lock<upgrade_lock> sl(l);
    Q_EMIT parent.blocksNeedUpdating(start, end+n);
}

CategoryProxyModelAttached* CategoryProxyModel::qmlAttachedProperties(QObject* object) {
    return new CategoryProxyModelAttached(object);
}

CategoryProxyModelAttached::CategoryProxyModelAttached(QObject* parent) : QObject(parent) {
    assert(parent != nullptr);

    auto v = parent->property("categoryModel");
    if(v == QVariant())
        return;
    QObject* obj = qvariant_cast<QObject*>(v);

    m_model = qobject_cast<CategoryProxyModel*>(obj);
    if(!m_model)
        return;
    modelConns.push_back(connect(m_model, &CategoryProxyModel::rowsAboutToBeRemoved, [this] (const QModelIndex&, int start, int end) {
        if(m_index.row() >= start && m_index.row() <= end) {
            setBlock(nullptr);
        }
    }));
    modelConns.push_back(connect(m_model, &CategoryProxyModel::blocksNeedUpdating, [this] (int start, int end) {
        if(m_index.row() >= start && m_index.row() <= end) {
            upgrade_lock l(m_model->pimpl->mu);
            TRACE_LOG(logject) << "updating block; index: " << m_index.row();
            auto b = m_model->pimpl->block_index[m_index.row()];
            l.unlock();
            setBlock(b);
        }
    }));

    v = parent->property("rowIndex");
    bool b;
    auto i = v.toInt(&b);
    if(v == QVariant() || !b)
        return;
    TRACE_LOG(logject) << "i: " << i;
    m_index = m_model->index(i, 0);
    TRACE_LOG(logject) << "index: " << m_index.isValid() << " " << m_index.row();
    assert(m_index.isValid());
    upgrade_lock l(m_model->pimpl->mu);
    m_block = m_model->pimpl->block_index[m_index.row()];
}

CategoryProxyModelAttached::~CategoryProxyModelAttached() {
    setBlock(nullptr);
    for(auto& c : modelConns)
        disconnect(c);
    if(m_model)
        TRACE_LOG(logject) << "attached object being destroyed";
}

Block* CategoryProxyModelAttached::block() const {
    return m_block.get();
}

void CategoryProxyModelAttached::setBlock(std::shared_ptr<Block> b) {
    m_block = b;
    Q_EMIT blockChanged(m_block.get());
}

Block::~Block() {
    setFirstIndex(QModelIndex());
    setCount(0);
}

bool Block::operator==(const Block& b) const {
    return m_firstIndex == b.m_firstIndex;
}

bool Block::adjacent(const Block& b) {
    return (firstRow() + m_count - b.firstRow() == 0) || (b.firstRow() + b.count() - firstRow() == 0);
}

bool Block::collapsed() const {
    return m_collapsed;
}

void Block::setCollapsed(bool c) {
    m_collapsed = c;
    Q_EMIT collapsedChanged(c);
}

int Block::count() const {
    return m_count;
}

void Block::setCount(int count) {
    assert(count >= 0);
    m_count = count;
    Q_EMIT countChanged(m_count);
}

QPersistentModelIndex Block::firstIndex() const {
    return m_firstIndex;
}

void Block::setFirstIndex(QPersistentModelIndex index) {
    if(!index.isValid())
        return;
    m_firstIndex = index;
    Q_EMIT firstIndexChanged(m_firstIndex);
    Q_EMIT firstRowChanged(m_firstIndex.row());
}

int Block::firstRow() const {
    return m_firstIndex.row();
}

} // namespace Melosic
