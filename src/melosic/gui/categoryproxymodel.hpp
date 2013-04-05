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

namespace Melosic {

class Block;
class Category;

class CategoryProxyModel : public QIdentityProxyModel {
    Q_OBJECT
    QMultiHash<QString, QSharedPointer<Block>> blocks;
    QSharedPointer<Block> blockForIndex_(const QModelIndex&);
    Q_PROPERTY(Melosic::Category* category MEMBER category NOTIFY categoryChanged)
    Category* category = nullptr;

public:
    explicit CategoryProxyModel(QObject* parent = nullptr);

    Q_INVOKABLE bool isFirstInBlock(const QModelIndex& index);
    Q_INVOKABLE bool blockIsCollapsed(const QModelIndex& index);
    Q_INVOKABLE void collapseBlockToggle(const QModelIndex& index);
    Q_INVOKABLE int itemsInBlock(const QModelIndex& index);

    Q_INVOKABLE QObject* blockForIndex(const QModelIndex& index);

    Q_INVOKABLE QString indexCategory(const QModelIndex& index) const;

Q_SIGNALS:
    void categoryChanged(const Category&);
};

class Block : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool collapsed READ collapsed WRITE setCollapsed NOTIFY collapsedChanged)
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    bool collapsed_ = false;
    int count_ = 0;

public:
    bool operator==(const Block& b) const {
        return firstIndex == b.firstIndex;
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

    Q_INVOKABLE int firstRow() const {
        return firstIndex.row();
    }

    QPersistentModelIndex firstIndex = QModelIndex();

Q_SIGNALS:
    void collapsedChanged(bool collapsed);
    void countChanged(int count);
};


} // namespace Melosic

#endif // MELOSIC_CATEGORYPROXYMODEL_HPP
