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

#ifndef MELOSIC_CATEGORYPROXYMODEL_HPP
#define MELOSIC_CATEGORYPROXYMODEL_HPP

#include <QIdentityProxyModel>
#include <QSharedPointer>
#include <qqml.h>

namespace Melosic {

class Block;
class Category;

class CategoryProxyModelAttached;

class CategoryProxyModel : public QIdentityProxyModel {
    Q_OBJECT
    QMultiHash<QString, QSharedPointer<Block>> blocks;

    QSharedPointer<Block> blockForIndex_(const QModelIndex&);
    bool hasBlock(const QModelIndex&);
    QSharedPointer<Block> merge(QSharedPointer<Block> a, QSharedPointer<Block> b);

    Q_PROPERTY(Melosic::Category* category MEMBER category NOTIFY categoryChanged)
    Category* category = nullptr;

    friend class CategoryProxyModelAttached;

public:
    explicit CategoryProxyModel(QObject* parent = nullptr);

    QString indexCategory(const QModelIndex& index) const;

    static CategoryProxyModelAttached* qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void categoryChanged(const Category& category);
    void blocksNeedUpdating(int start, int end);

private Q_SLOTS:
    void onRowsInserted(const QModelIndex&, int start, int end);
    void onRowsMoved(const QModelIndex&, int sourceStart, int sourceEnd, const QModelIndex&, int destinationRow);
    void onRowsAboutToBeMoved(const QModelIndex&, int sourceStart, int sourceEnd,
                              const QModelIndex&, int destinationRow);
    void onRowsRemoved(const QModelIndex&, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex&, int start, int end);
};

class CategoryProxyModelAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(Melosic::Block* block READ block NOTIFY blockChanged)
    mutable QSharedPointer<Block> block_;
    CategoryProxyModel* model;
    QList<QMetaObject::Connection> modelConns;

    void setBlock(QSharedPointer<Block> b);
    QPersistentModelIndex index;

public:
    explicit CategoryProxyModelAttached(QObject* parent);
    ~CategoryProxyModelAttached();

    Block* block() const;

Q_SIGNALS:
    void blockChanged(Block*);
};

class Block : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool collapsed READ collapsed WRITE setCollapsed NOTIFY collapsedChanged)
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(QPersistentModelIndex firstIndex READ firstIndex WRITE setFirstIndex NOTIFY firstIndexChanged)
    Q_PROPERTY(int firstRow READ firstRow NOTIFY firstRowChanged)
    bool collapsed_ = false;
    int count_ = 0;
    QPersistentModelIndex firstIndex_ = QModelIndex();
    QString category;
    friend class CategoryProxyModel;

public:
    ~Block();

    bool operator==(const Block& b) const {
        return firstIndex_ == b.firstIndex_;
    }

    bool adjacent(const Block& b);

    bool collapsed() const {
        return collapsed_;
    }

    void setCollapsed(bool c) {
        collapsed_ = c;
        Q_EMIT collapsedChanged(c);
    }

    int count() const {
        return count_;
    }

    void setCount(int count) {
        count_ = count;
        Q_EMIT countChanged(count_);
    }

    QPersistentModelIndex firstIndex() const {
        return firstIndex_;
    }

    void setFirstIndex(QPersistentModelIndex index) {
        firstIndex_ = index;
        if(category.size() == 0)
            category = qobject_cast<CategoryProxyModel*>(const_cast<QAbstractItemModel*>(index.model()))->
                    indexCategory(firstIndex_);
        Q_EMIT firstIndexChanged(firstIndex_);
        Q_EMIT firstRowChanged(firstIndex_.row());
    }

    int firstRow() const {
        return firstIndex_.row();
    }

Q_SIGNALS:
    void collapsedChanged(bool collapsed);
    void countChanged(int count);
    void firstIndexChanged(QPersistentModelIndex index);
    void firstRowChanged(int row);
};

} // namespace Melosic

QML_DECLARE_TYPEINFO(Melosic::CategoryProxyModel, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(Melosic::Block)

#endif // MELOSIC_CATEGORYPROXYMODEL_HPP
