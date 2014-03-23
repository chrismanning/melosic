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

#include <memory>

#include <QIdentityProxyModel>
#include <qqml.h>

namespace Melosic {

class Block;
class Category;

class CategoryProxyModelAttached;

class CategoryProxyModel : public QIdentityProxyModel {
    Q_OBJECT

    Q_PROPERTY(Melosic::Category* category READ category WRITE setCategory NOTIFY categoryChanged)

    friend class CategoryProxyModelAttached;

public:
    explicit CategoryProxyModel(QObject* parent = nullptr);
    ~CategoryProxyModel();

    QString indexCategory(const QModelIndex& index) const;

    Category* category() const;
    void setCategory(Category*);

    static CategoryProxyModelAttached* qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void categoryChanged(Melosic::Category* category);
    void blocksNeedUpdating(int start, int end);

private:
    void onRowsInserted(const QModelIndex&, int start, int end);
    void onRowsMoved(const QModelIndex&, int sourceStart, int sourceEnd, const QModelIndex&, int destinationRow);
    void onRowsAboutToBeMoved(const QModelIndex&, int sourceStart, int sourceEnd,
                              const QModelIndex&, int destinationRow);
    void onRowsRemoved(const QModelIndex&, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex&, int start, int end);

    void onDataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&);

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
    friend class Block;
};

class CategoryProxyModelAttached : public QObject {
    Q_OBJECT

    Q_PROPERTY(Melosic::Block* block READ block NOTIFY blockChanged)
    Q_PROPERTY(bool drawCategory READ drawCategory NOTIFY drawCategoryChanged)
    Q_PROPERTY(CategoryProxyModel* model READ model NOTIFY modelChanged)

    mutable std::shared_ptr<Block> m_block;
    CategoryProxyModel* m_model{nullptr};
    QList<QMetaObject::Connection> modelConns;
    QPersistentModelIndex m_index;

    Block* block() const;
    bool drawCategory() const;
    CategoryProxyModel* model() const;

    void setModel(CategoryProxyModel* model);
    void setBlock(std::shared_ptr<Block> b);

    void internal_update();

private Q_SLOTS:
    void setModelFromParent();

public:
    explicit CategoryProxyModelAttached(QObject* parent);
    ~CategoryProxyModelAttached();

Q_SIGNALS:
    void blockChanged(Block* block);
    void drawCategoryChanged(bool drawCategory);
    void modelChanged(CategoryProxyModel* model);
};

class Block : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool collapsed READ collapsed WRITE setCollapsed NOTIFY collapsedChanged)
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(QPersistentModelIndex firstIndex READ firstIndex WRITE setFirstIndex NOTIFY firstIndexChanged)
    Q_PROPERTY(int firstRow READ firstRow NOTIFY firstRowChanged)
    bool m_collapsed = false;
    int m_count = 0;
    QPersistentModelIndex m_firstIndex = QModelIndex();
    friend class CategoryProxyModel;

public:
    ~Block();

    bool operator==(const Block& b) const;

    bool adjacent(const Block& b);
    bool collapsed() const;
    int count() const;
    QPersistentModelIndex firstIndex() const;
    int firstRow() const;

    // emits collapsedChanged
    void setCollapsed(bool c);
    // emits countChanged
    void setCount(int count);
    // emits firstIndexChanged & firstRowChanged
    void setFirstIndex(QPersistentModelIndex index);

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
