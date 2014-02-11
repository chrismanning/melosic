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

#include <mutex>
#include <shared_mutex>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
using mutex = boost::shared_mutex;
using shared_lock = boost::shared_lock<mutex>;
using unique_lock = boost::unique_lock<mutex>;
using upgrade_lock = boost::upgrade_lock<mutex>;

#include <QSharedPointer>
#include <QDebug>

#include <melosic/melin/logging.hpp>
#include <melosic/common/scope_unlock_exit_lock.hpp>

#include "categoryproxymodel.hpp"
#include "category.hpp"

namespace Melosic {

Logger::Logger logject{logging::keywords::channel = "CategoryProxyModel"};

struct CategoryProxyModel::impl {
    explicit impl(CategoryProxyModel& parent) : parent(parent) {}

    QMultiHash<QString, QSharedPointer<Block>> blocks;
    mutable mutex mu;

    QSharedPointer<Block> blockForIndex_(const QModelIndex&, upgrade_lock&);
    bool hasBlock(const QModelIndex&, upgrade_lock&);
    QSharedPointer<Block> merge(QSharedPointer<Block> a, QSharedPointer<Block> b, upgrade_lock&);
    QString indexCategory(const QModelIndex& index, upgrade_lock&) const;

    void onRowsInserted(const QModelIndex&, int start, int end, upgrade_lock& l);
    void onRowsMoved(const QModelIndex&, int sourceStart, int sourceEnd, const QModelIndex&, int destinationRow, upgrade_lock& l);
    void onRowsAboutToBeMoved(const QModelIndex&, int sourceStart, int sourceEnd,
                              const QModelIndex&, int destinationRow, upgrade_lock& l);
    void onRowsRemoved(const QModelIndex&, int start, int end, upgrade_lock& l);
    void onRowsAboutToBeRemoved(const QModelIndex&, int start, int end, upgrade_lock& l);

    void onDataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&, upgrade_lock& l);

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
        Q_ASSERT(c);
        Q_ASSERT(c->model());
        onDataChanged(index(0,0), index(sourceModel()->rowCount() - 1, 0), {});
    });
}

CategoryProxyModel::~CategoryProxyModel() {}

QString CategoryProxyModel::indexCategory(const QModelIndex& index) const {
    upgrade_lock l(pimpl->mu);
    return pimpl->indexCategory(index, l);
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
        Q_ASSERT(c->model() == this);
    l.unlock();
    Q_EMIT categoryChanged(c);
}

void CategoryProxyModel::onRowsInserted(const QModelIndex& p, int start, int end) {
    upgrade_lock l(pimpl->mu);
    pimpl->onRowsInserted(p, start, end, l);
}

void CategoryProxyModel::onRowsMoved(const QModelIndex& sp,
                                     int sourceStart,
                                     int sourceEnd,
                                     const QModelIndex& dp,
                                     int destinationRow)
{
    upgrade_lock l(pimpl->mu);
    pimpl->onRowsMoved(sp, sourceStart, sourceEnd, dp, destinationRow, l);
}

void CategoryProxyModel::onRowsAboutToBeMoved(const QModelIndex& sp,
                                              int sourceStart,
                                              int sourceEnd,
                                              const QModelIndex& dp,
                                              int destinationRow)
{
    upgrade_lock l(pimpl->mu);
    pimpl->onRowsAboutToBeMoved(sp, sourceStart, sourceEnd, dp, destinationRow, l);
}

void CategoryProxyModel::onRowsRemoved(const QModelIndex& p, int start, int end) {
    upgrade_lock l(pimpl->mu);
    pimpl->onRowsRemoved(p, start, end, l);
}

void CategoryProxyModel::onRowsAboutToBeRemoved(const QModelIndex& p, int start, int end) {
    upgrade_lock l(pimpl->mu);
    pimpl->onRowsAboutToBeRemoved(p, start, end, l);
}

void CategoryProxyModel::onDataChanged(const QModelIndex& topleft,
                                       const QModelIndex& bottomright,
                                       const QVector<int>& roles)
{
    upgrade_lock l(pimpl->mu);
    pimpl->onDataChanged(topleft, bottomright, roles, l);
}

QSharedPointer<Block>
CategoryProxyModel::impl::blockForIndex_(const QModelIndex& index, upgrade_lock& l) {
    if(!m_category)
        return {};

    Q_ASSERT(index.isValid());

    TRACE_LOG(logject) << "blockForIndex " << index.row();

    const QString category_(indexCategory(index, l));
    for(const QSharedPointer<Block>& block : blocks.values(category_)) {
        if(block->count() == 0 || !block->firstIndex().isValid()) {
            TRACE_LOG(logject) << (block->count() == 0 ? "block empty" : "block has invalid first index");
            {
                boost::upgrade_to_unique_lock<mutex> ul(l);
                blocks.remove(category_, block);
            }
            return blockForIndex_(index, l);
        }

        TRACE_LOG(logject) << "block->count(): " << block->count();
        //in the middle of block
        if(block->contains(index)) {
            Q_ASSERT(block->count() > 0);
            return block;
        }
    }

    TRACE_LOG(logject) << "creating new block for index " << index.row();

    //create new block
    QSharedPointer<Block> empty(new Block);
    empty->setFirstIndex(index, l);
    {
        auto s = blocks.size();
        boost::upgrade_to_unique_lock<mutex> ul(l);
        empty = *blocks.insert(category_, empty);
        Q_ASSERT(blocks.size() == s+1);
        empty->setCount(empty->count() + 1);
        Q_ASSERT(empty->count() > 0);
    }
    return empty;
}

bool CategoryProxyModel::impl::hasBlock(const QModelIndex& index, upgrade_lock& l) {
    if(!index.isValid())
        return false;

    const QString category_(indexCategory(index, l));
    for(QSharedPointer<Block> block : blocks.values(category_)) {
        if(index.row() >= block->firstRow() && index.row() < block->firstRow() + block->count())
            return true;
    }

    return false;
}

QSharedPointer<Block>
CategoryProxyModel::impl::merge(QSharedPointer<Block> big, QSharedPointer<Block> small, upgrade_lock& l) {
    Q_ASSERT(big);
    Q_ASSERT(small);
    Q_ASSERT(big->adjacent(*small));
    Q_ASSERT(big != small);
    TRACE_LOG(logject) << "Merging blocks";
    if(big->count() < small->count())
        big.swap(small);

    if(small->firstIndex() < big->firstIndex())
        big->setFirstIndex(small->firstIndex(), l);
    big->setCount(big->count() + small->count());
    auto cat = indexCategory(small->firstIndex(), l);
    {
        boost::upgrade_to_unique_lock<mutex> ul(l);
        blocks.remove(cat, small);
    }

    l.unlock();
    Q_EMIT parent.blocksNeedUpdating(big->firstRow(), big->firstRow() + big->count());

    return big;
}

QString CategoryProxyModel::impl::indexCategory(const QModelIndex& index, upgrade_lock&) const {
    Q_ASSERT(index.isValid());

    if(!m_category)
        return "?";

    QString cat;
    for(Criteria* c : m_category->criteria_)
        cat += c->result(index);
    return cat;
}

void CategoryProxyModel::impl::onRowsInserted(const QModelIndex&, int s, int e, upgrade_lock &l) {
    if(!m_category)
        return;
    TRACE_LOG(logject) << "rows inserted " << s << " - " << e;
}

void CategoryProxyModel::impl::onRowsMoved(const QModelIndex& parent, int sourceStart,
                                           int sourceEnd, const QModelIndex&,
                                           int destinationRow, upgrade_lock& l)
{
    Q_ASSERT(sourceEnd >= sourceStart);
    auto s = sourceEnd - sourceStart;
    auto dest = destinationRow < sourceStart ? destinationRow : destinationRow - (s + 1);

    onDataChanged(this->parent.index(dest, 0), this->parent.index(dest+s, 0), {}, l);
}

void CategoryProxyModel::impl::onRowsAboutToBeMoved(const QModelIndex& parent, int sourceStart, int sourceEnd,
                                                    const QModelIndex&, int, upgrade_lock& l) {
    onRowsAboutToBeRemoved(parent, sourceStart, sourceEnd, l);
}

void CategoryProxyModel::impl::onRowsRemoved(const QModelIndex&,
                                             int start, int end,
                                             upgrade_lock& l)
{
    if(!m_category || parent.rowCount() == 0) {
        blocks.clear();
        return;
    }
    TRACE_LOG(logject) << "Rows removed: " << start << " - " << end;
    QSharedPointer<Block> a,b;
    if(start > 0 && start < parent.rowCount()) {
        a = blockForIndex_(parent.index(start - 1, 0), l);
        b = blockForIndex_(parent.index(start, 0), l);
        if(a && b && a != b && indexCategory(a->firstIndex(), l) == indexCategory(b->firstIndex(), l))
            merge(a, b, l);
    }
    if(start < parent.rowCount() - 1) {
        a = blockForIndex_(parent.index(start, 0), l);
        b = blockForIndex_(parent.index(start + 1, 0), l);
        if(a && b && a != b && indexCategory(a->firstIndex(), l) == indexCategory(b->firstIndex(), l))
            merge(a, b, l);
    }
}

void CategoryProxyModel::impl::onRowsAboutToBeRemoved(const QModelIndex&, int start, int end, upgrade_lock& l) {
    if(!m_category)
        return;
    TRACE_LOG(logject) << "Rows about to be removed: " << start << " - " << end;

    for(int i = end; i >= start; i--) {
        auto cur = parent.index(i, 0);
        Q_ASSERT(cur.isValid());
        auto b = blockForIndex_(cur, l);
        if(b->count() <= 1) {
            TRACE_LOG(logject) << "removing empty block";
            auto cat = indexCategory(cur, l);
            b->setFirstIndex(QModelIndex(), l);
            boost::upgrade_to_unique_lock<mutex> ul(l);
            blocks.remove(cat, b);
            b->setCount(0);
            continue;
        }
        TRACE_LOG(logject) << "decrementing block count";
        b->setCount(b->count() - 1);
        if(b->firstRow() == cur.row()) {
            auto n = end - start + 1;
            TRACE_LOG(logject) << "moving block forward " << n;
            b->setFirstIndex(parent.index(i+n, 0), l);
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
    Q_ASSERT(l.owns_lock());

    TRACE_LOG(logject) << "data changed; rows " << istart.row() << " - " << iend.row();

    auto start = istart.row(), end = iend.row();
    QModelIndex prev;
    QString prevCategory;
    QSharedPointer<Block> prevBlock;
    int end_ = end;

    if(start > 0) {
        prev = parent.index(start-1, 0);
        Q_ASSERT(prev.isValid());
        prevBlock = blockForIndex_(prev, l);
        Q_ASSERT(prevBlock);
        prevCategory = indexCategory(prev, l);
        auto d = prevBlock->firstRow() + prevBlock->count();
        if(d > start) {
            TRACE_LOG(logject) << "split block";
            auto c = prevBlock->count();
            auto c2 = d - start;

            prevBlock->setCount(c - c2);

            auto b = blockForIndex_(parent.index(end+1, 0), l);
            b->setCount(c2);

            Q_ASSERT((d-prevBlock->firstRow()) == (b->count() + prevBlock->count()));
            end_ += b->count() + 1;
        }
    }

    for(int i = start; i <= end; i++) {
        const QModelIndex cur(parent.index(i, 0));
        Q_ASSERT(cur.isValid());
        const QString curCategory(indexCategory(cur, l));
        QSharedPointer<Block> curBlock;

        if(prevBlock && curCategory == prevCategory && !prevBlock->contains(cur)) {
            if(hasBlock(cur, l)) {
                TRACE_LOG(logject) << "removing block";
                boost::upgrade_to_unique_lock<mutex> ul(l);
                blocks.remove(curCategory, curBlock = blockForIndex_(cur, l));
            }
            boost::upgrade_to_unique_lock<mutex> ul(l);
            prevBlock->setCount(prevBlock->count() + 1);
        }
        if(!curBlock)
            curBlock = blockForIndex_(cur, l);
        Q_ASSERT(curBlock);
        if(curBlock->firstIndex() == cur && prevBlock && prevBlock->contains(cur))
            prevBlock->setCount(prevBlock->count()-1);

        prev = cur;
        prevCategory = curCategory;
        prevBlock = curBlock;
    }

    if(end + 1 < parent.rowCount(QModelIndex()) && hasBlock(parent.index(end + 1, 0), l)) {
        const QModelIndex epo(parent.index(end + 1, 0));
        if(hasBlock(epo, l) && prevCategory == indexCategory(epo, l)) {
            TRACE_LOG(logject) << "merging at end of inserted";
            const auto epob = blockForIndex_(epo, l);
            Q_ASSERT(epob);
            Q_ASSERT(epob != prevBlock);
            boost::upgrade_to_unique_lock<mutex> ul(l);
            prevBlock->setCount(prevBlock->count() + epob->count());
            blocks.remove(prevCategory, epob);
            end_ += 1+epob->count();
        }
        else {

        }
    }

    scope_unlock_exit_lock<upgrade_lock> s{l};
    Q_EMIT parent.blocksNeedUpdating(start, end_);
}

CategoryProxyModelAttached* CategoryProxyModel::qmlAttachedProperties(QObject* object) {
    return new CategoryProxyModelAttached(object);
}

CategoryProxyModelAttached::CategoryProxyModelAttached(QObject* parent) : QObject(parent) {
    Q_ASSERT(parent != nullptr);

    auto v = parent->property("categoryModel");
    if(v == QVariant())
        return;
    QObject* obj = qvariant_cast<QObject*>(v);

    model = qobject_cast<CategoryProxyModel*>(obj);
    if(!model)
        return;
    modelConns.push_back(connect(model, &CategoryProxyModel::rowsAboutToBeRemoved, [this] (const QModelIndex&, int start, int end) {
        if(index.row() >= start && index.row() <= end) {
            setBlock({});
        }
    }));
    modelConns.push_back(connect(model, &CategoryProxyModel::blocksNeedUpdating, [this] (int start, int end) {
        if(index.row() >= start && index.row() <= end) {
            upgrade_lock l(model->pimpl->mu);
            TRACE_LOG(logject) << "updating block; index: " << index.row();
            auto b = model->pimpl->blockForIndex_(index, l);
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
    index = model->index(i, 0);
    TRACE_LOG(logject) << "index: " << index.isValid() << " " << index.row();
    Q_ASSERT(index.isValid());
    upgrade_lock l(model->pimpl->mu);
    if(model->pimpl->hasBlock(index, l))
        m_block = model->pimpl->blockForIndex_(index, l);
}

CategoryProxyModelAttached::~CategoryProxyModelAttached() {
    setBlock({});
    for(auto& c : modelConns)
        disconnect(c);
    if(model)
        TRACE_LOG(logject) << "attached object being destroyed";
}

Block* CategoryProxyModelAttached::block() const {
    return m_block.data();
}

void CategoryProxyModelAttached::setBlock(QSharedPointer<Block> b) {
    m_block = b;
    Q_EMIT blockChanged(m_block.data());
}

bool Block::adjacent(const Block& b) {
    return (firstRow() + count_ - b.firstRow() == 0) || (b.firstRow() + b.count() - firstRow() == 0);
}

template <typename UpgradeableLock>
void Block::setFirstIndex(QPersistentModelIndex index, UpgradeableLock& l) {
    if(!index.isValid())
        return;
    firstIndex_ = index;
    if(category.size() == 0)
        category = qobject_cast<CategoryProxyModel*>(const_cast<QAbstractItemModel*>(index.model()))->
                pimpl->indexCategory(firstIndex_, l);
    Q_EMIT firstIndexChanged(firstIndex_);
    Q_EMIT firstRowChanged(firstIndex_.row());
}

template void Block::setFirstIndex<upgrade_lock>(QPersistentModelIndex index, upgrade_lock& l);

Block::~Block() {
    setFirstIndex(QModelIndex());
    setCount(0);
}

} // namespace Melosic
