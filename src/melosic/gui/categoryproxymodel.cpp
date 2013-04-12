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

#include <QSharedPointer>
#include <QDebug>

#include "categoryproxymodel.hpp"
#include "category.hpp"

namespace Melosic {

CategoryProxyModel::CategoryProxyModel(QObject* parent) : QIdentityProxyModel(parent) {
    connect(this, &CategoryProxyModel::rowsInserted, this, &CategoryProxyModel::onRowsInserted);
    connect(this, &CategoryProxyModel::rowsMoved, this, &CategoryProxyModel::onRowsMoved);
    connect(this, &CategoryProxyModel::rowsRemoved, this, &CategoryProxyModel::onRowsRemoved);
    connect(this, &CategoryProxyModel::rowsAboutToBeRemoved, this, &CategoryProxyModel::onRowsAboutToBeRemoved);
}

QSharedPointer<Block> CategoryProxyModel::blockForIndex_(const QModelIndex& index) {
    if(!category)
        return {};

    Q_ASSERT(index.isValid());

    const QString category_(indexCategory(index));
    for(QSharedPointer<Block> block : blocks.values(category_)) {
        if(block->count() == 0 || !block->firstIndex().isValid()) {
            blocks.remove(category_, block);
            return blockForIndex_(index);
        }

        //in the middle of block
        if(block->firstRow() <= index.row() && block->firstRow() + block->count() > index.row()) {
            Q_ASSERT(block->count() > 0);
            return block;
        }
    }

    //create new block
    QSharedPointer<Block> empty(new Block);
    empty->setFirstIndex(index);
    auto s = blocks.size();
    auto b = *blocks.insert(category_, empty);
    Q_ASSERT(blocks.size() == s+1);
    b->setCount(b->count() + 1);
    Q_ASSERT(b->count() > 0);
    return b;
}

bool CategoryProxyModel::hasBlock(const QModelIndex& index) {
    if(!index.isValid())
        return false;

    const QString category_(indexCategory(index));
    for(QSharedPointer<Block> block : blocks.values(category_)) {
        if(index.row() >= block->firstRow() && index.row() < block->firstRow() + block->count())
            return true;
    }

    return false;
}

QSharedPointer<Block> CategoryProxyModel::merge(QSharedPointer<Block> big, QSharedPointer<Block> small) {
    Q_ASSERT(big);
    Q_ASSERT(small);
    Q_ASSERT(big->adjacent(*small));
    Q_ASSERT(big != small);
    qDebug() << "Merging blocks";
    if(big->count() < small->count())
        big.swap(small);

    if(small->firstIndex() < big->firstIndex())
        big->setFirstIndex(small->firstIndex());
    big->setCount(big->count() + small->count());
    blocks.remove(indexCategory(big->firstIndex()), small);

    Q_EMIT blocksNeedUpdating(big->firstRow(), big->firstRow() + big->count());

    return big;
}

QString CategoryProxyModel::indexCategory(const QModelIndex& index) const {
    Q_ASSERT(index.isValid());
    if(!category)
        return "?";
    QString cat;
    for(Criteria* c : category->criteria_)
        cat += c->result(index);
    return cat;
}

void CategoryProxyModel::onRowsInserted(const QModelIndex& parent, int start, int end) {
    if(!category)
        return;

    QModelIndex prev;
    QString prevCategory;
    QSharedPointer<Block> prevBlock;
    int end_ = end;

    if(start > 0) {
        prev = index(start-1, 0);
        Q_ASSERT(prev.isValid());
        prevBlock = blockForIndex_(prev);
        Q_ASSERT(prevBlock);
        prevCategory = indexCategory(prev);
        auto d = prevBlock->firstRow() + prevBlock->count();
        if(d > start) {
            //split block
            auto c = prevBlock->count();
            auto c2 = d - start;
            prevBlock->setCount(c - c2);
            auto b = blockForIndex_(index(end+1, 0));
            b->setCount(c2);
            Q_ASSERT((d-prevBlock->firstRow()) == (b->count() + prevBlock->count()));
            end_ += b->count() + 1;
        }
    }

    for(int i = start; i <= end; i++) {
        const QModelIndex cur(index(i, 0));
        Q_ASSERT(cur.isValid());
        const QString curCategory(indexCategory(cur));

        if(prevBlock && curCategory == prevCategory) {
            if(hasBlock(cur)) {
                auto b = blockForIndex_(cur);
                blocks.remove(curCategory, b);
            }
            prevBlock->setCount(prevBlock->count() + 1);
        }

        prev = cur;
        prevCategory = curCategory;
        prevBlock = blockForIndex_(cur);
    }

    if(end + 1 < rowCount(parent)) {
        const QModelIndex epo(index(end + 1, 0));
        if(prevCategory == indexCategory(epo)) {
            //merging at end of inserted
            auto epob = blockForIndex_(epo);
            Q_ASSERT(epob);
            Q_ASSERT(epob != prevBlock);
            prevBlock->setCount(prevBlock->count() + epob->count());
            blocks.remove(prevCategory, epob);
            Q_EMIT blocksNeedUpdating(end+1, end+1+epob->count());
        }
        else {

        }
    }
    Q_EMIT blocksNeedUpdating(start, end_);
}

void CategoryProxyModel::onRowsMoved(const QModelIndex&, int /*sourceStart*/,
                                     int /*sourceEnd*/, const QModelIndex&, int /*destinationRow*/) {
}

void CategoryProxyModel::onRowsRemoved(const QModelIndex&, int start, int /*end*/) {
    if(!category || rowCount() == 0)
        return;

    QSharedPointer<Block> a,b;
    if(start > 0 && start < rowCount()) {
        a = blockForIndex_(index(start - 1, 0));
        b = blockForIndex_(index(start, 0));
    }
    else if(start < rowCount() - 1) {
        a = blockForIndex_(index(start, 0));
        b = blockForIndex_(index(start + 1, 0));
    }
    qDebug() << "a != b: " << (a != b);
    if(a)
        qDebug() << "indexCategory(a->firstIndex()): " << indexCategory(a->firstIndex());
    if(b)
        qDebug() << "indexCategory(b->firstIndex()): " << indexCategory(b->firstIndex());
    if(a && b && a != b && indexCategory(a->firstIndex()) == indexCategory(b->firstIndex()))
        merge(a, b);
}

void CategoryProxyModel::onRowsAboutToBeRemoved(const QModelIndex&, int start, int end) {
    if(!category)
        return;
    qDebug() << "Rows removed: " << start << " - " << end;

    for(int i = start; i <= end; i++) {
        auto cur = index(i, 0);
        Q_ASSERT(cur.isValid());
        auto b = blockForIndex_(cur);
        if(b->count() <= 1) {
            qDebug() << "removing empty block";
            blocks.remove(indexCategory(cur), b);
            b->setFirstIndex(QModelIndex());
            b->setCount(0);
            continue;
        }
        qDebug() << "decrementing block count";
        b->setCount(b->count() - 1);
        if(b->firstRow() == cur.row()) {
            qDebug() << "moving block forward 1";
            b->setFirstIndex(index(i+1, 0));
        }
    }

    ++end;
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
            qDebug() << "updating block; index: " << index.row();
            setBlock(model->blockForIndex_(index));
        }
    }));

    v = parent->property("rowIndex");
    bool b;
    auto i = v.toInt(&b);
    if(v == QVariant() || !b)
        return;
    index = model->index(i, 0);
    Q_ASSERT(index.isValid());
    if(model->hasBlock(index))
        block_ = model->blockForIndex_(index);
}

CategoryProxyModelAttached::~CategoryProxyModelAttached() {
    setBlock({});
    for(auto& c : modelConns)
        disconnect(c);
    qDebug() << "attached object being destroyed";
}

Block* CategoryProxyModelAttached::block() const {
    return block_.data();
}

void CategoryProxyModelAttached::setBlock(QSharedPointer<Block> b) {
    block_ = b;
    Q_EMIT blockChanged(block_.data());
}

bool Block::adjacent(const Block& b) {
    return (firstRow() + count_ - b.firstRow() == 0) || (b.firstRow() + b.count() - firstRow() == 0);
}

Block::~Block() {
    setFirstIndex(QModelIndex());
    setCount(0);
}

} // namespace Melosic
