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

CategoryProxyModel::CategoryProxyModel(QObject* parent) : QIdentityProxyModel(parent) {}

QSharedPointer<Block> CategoryProxyModel::blockForIndex_(const QModelIndex& index) {
    if(!category)
        return {};
    const QString category_(indexCategory(index));
    Q_ASSERT(index.isValid());
    for(QSharedPointer<Block> block : blocks.values(category_)) {
        if(block->count() == 0 || !block->firstIndex.isValid()) {
            blocks.remove(category_, block);
            return blockForIndex_(index);
        }

        //search backwards
        if(block->firstIndex == index && index.row() > 0) {
            auto b2 = blockForIndex_(this->index(index.row() - 1, index.column()));
            if(b2->count() > (block->firstIndex.row() - b2->firstIndex.row())) {
                qDebug() << "block inside block";
                if(indexCategory(b2->firstIndex) != category_) {
                    qDebug() << "seperating block";
                    b2->setCount(block->firstIndex.row() - b2->firstIndex.row());
                    qDebug() << "b2->count(): " << b2->count();
                    auto i = block->firstIndex.row() + block->count();
                    blockForIndex_(this->index(i, index.column()));
                    return block;
                }
            }
        }

        //merge backwards
        if(block->firstRow() - 1 == index.row()) {
            qDebug() << "merging backwards";
            block->firstIndex = index;
            block->setCount(block->count() + 1);
//            return block;
        }

        //merge forwards
        if(block->firstRow() + block->count() == index.row()) {
            qDebug() << "merging forwards";
            block->setCount(block->count() + 1);
//            return block;
        }

        //in the middle of block
        if(block->firstRow() <= index.row() && block->firstRow() + block->count() > index.row()) {
            qDebug() << "found associated block";

            //search forward
            auto v = block->firstRow() + block->count();

            QModelIndex i2;
            if(v < rowCount())
                i2 = this->index(v, index.column());

            if(i2.isValid() && indexCategory(i2) == category_) {
                auto b2 = blockForIndex_(i2);
                if(b2 != block) {
                    qDebug() << "merging blocks";
                    Q_ASSERT(b2->firstIndex != block->firstIndex);
                    if(b2->firstRow() > block->firstRow()) {
                        block->setCount(block->count() + b2->count());
                        b2->setCount(0);
                        return block;
                    }
                    else {
                        b2->setCount(b2->count() + block->count());
                        block->setCount(0);
                        return b2;
                    }
                }
            }

            Q_ASSERT(block->count() > 0);
            return block;
        }
    }

    //create new block
    QSharedPointer<Block> empty(new Block);
    empty->firstIndex = index;
    auto s = blocks.size();
    auto b = *blocks.insert(category_, empty);
    Q_ASSERT(blocks.size() == s+1);
    b->setCount(b->count() + 1);
    Q_ASSERT(b->count() > 0);
    return b;
}

bool CategoryProxyModel::isFirstInBlock(const QModelIndex& index) {
    if(!category)
        return false;
    Q_ASSERT(index.row() >= 0 && index.row() < rowCount());
    return blockForIndex_(index)->firstIndex == index;
}

bool CategoryProxyModel::blockIsCollapsed(const QModelIndex& index) {
    if(!category)
        return false;
    Q_ASSERT(index.row() >= 0 && index.row() < rowCount());
    return blockForIndex_(index)->collapsed();
}

void CategoryProxyModel::collapseBlockToggle(const QModelIndex& index) {
    if(!category)
        return;
    Q_ASSERT(index.row() >= 0 && index.row() < rowCount());
    auto b = blockForIndex_(index);
    b->setCollapsed(!b->collapsed());
}

int CategoryProxyModel::itemsInBlock(const QModelIndex& index) {
    if(!category)
        return 0;
    Q_ASSERT(index.row() >= 0 && index.row() < rowCount());
    return blockForIndex_(index)->count();
}

QObject* CategoryProxyModel::blockForIndex(const QModelIndex& index) {
    return blockForIndex_(index).data();
}

QString CategoryProxyModel::indexCategory(const QModelIndex& index) const {
    if(!category)
        return "?";
    QString cat;
    for(Criteria* c : category->criteria_) {
        cat += c->result(index);
    }
    return cat;
}

bool Block::adjacent(const Block& b) {
    return (firstRow() + count_ - b.firstRow() == 0) || (b.firstRow() + b.count() - firstRow() == 0);
}

} // namespace Melosic
