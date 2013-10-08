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

#include <QRegExp>
#include <QDebug>

#include "category.hpp"
#include "categoryproxymodel.hpp"

namespace Melosic {

Category::Category(QObject* parent) : QObject(parent) {
    for(Criteria* c : criteria_)
        c->model = model();

    connect(this, &Category::modelChanged, [this] (CategoryProxyModel* m) {
        for(Criteria* c : criteria_)
            c->model = m;
    });
}

QQmlListProperty<Criteria> Category::categoryCriteria() {
    return {this, criteria_};
}

CategoryProxyModel* Category::model() const {
    return model_;
}

void Category::setModel(CategoryProxyModel* m) {
    model_ = m;
    Q_EMIT modelChanged(model_);
}

QQmlComponent* Category::delegate() const {
    return delegate_;
}

void Category::setDelegate(QQmlComponent* d) {
    delegate_ = d;
    Q_EMIT delegateChanged(d);
}

Criteria::Criteria(QObject* parent) : QObject(parent) {}

QVariant Criteria::pattern() const {
    return pattern_;
}

void Criteria::setPattern(QVariant p) {
    pattern_ = p;
    Q_EMIT patternChanged(p);
}

Role::Role(QObject* parent) : Criteria(parent) {}

QString Role::role() const {
    return role_;
}

void Role::setRole(QString str) {
    role_ = str;
    Q_EMIT roleChanged(role_);
}

QString Role::result(const QModelIndex& index) const {
    Q_ASSERT(index.isValid());
    if(!model) {
        return role_;
    }

    auto roles = model->roleNames();
    if(!roles.values().contains(role_.toUtf8()))
        return "?";
    return index.data(model->roleNames().key(role_.toUtf8())).toString();
}

Tag::Tag(QObject* parent) : Criteria(parent) {}

QString Tag::field() const {
    return field_;
}

void Tag::setField(QString str) {
    field_ = str;
    Q_EMIT fieldChanged(str);
}

QString Tag::result(const QModelIndex&) const {
    return tr("");
}

} // namespace Melosic
