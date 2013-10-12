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
    connect(this, &Category::modelChanged, [this] (CategoryProxyModel* m) {
        for(Criteria* c : criteria_)
            c->setModel(m);
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

Criteria::Criteria(QObject* parent) : QObject(parent) {
    connect(this, &Criteria::patternChanged, [this] (QString p) { m_regex.setPattern(p); });
}

QString Criteria::pattern() const {
    return m_pattern;
}

void Criteria::setPattern(QString p) {
    m_pattern = p;
    qDebug() << "Pattern set: " << p;
    Q_EMIT patternChanged(p);
}

void Criteria::setModel(CategoryProxyModel* model) {
    m_category_model = model;
    Q_EMIT modelChanged(m_category_model);
}

CategoryProxyModel* Criteria::model() const {
    return m_category_model;
}

Role::Role(QObject* parent) : Criteria(parent) {}

QString Role::role() const {
    return m_role;
}

void Role::setRole(QString str) {
    m_role = str;
    Q_EMIT roleChanged(m_role);
}

QString Role::result(const QModelIndex& index) const {
    Q_ASSERT(index.isValid());
    if(!m_category_model)
        return m_role;

    auto roles = m_category_model->roleNames();
    if(!roles.values().contains(m_role.toUtf8()))
        return m_role;
    auto res = index.data(m_category_model->roleNames().key(m_role.toUtf8())).toString();
    auto match = m_regex.match(res);
    return match.hasMatch() ? match.captured() : "";
}

} // namespace Melosic
