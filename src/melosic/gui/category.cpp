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

#include <QRegExp>

#include "category.hpp"
#include "categoryproxymodel.hpp"

namespace Melosic {

Category::Category(QObject* parent) : QObject(parent) {
    connect(this, &Category::modelChanged, [this](CategoryProxyModel* m) {
        for(Criterion* c : m_criteria)
            c->setModel(m);
    });
    if(parent && qobject_cast<CategoryProxyModel*>(parent))
        this->setProperty("model", QVariant::fromValue(parent));
}

QQmlListProperty<Criterion> Category::categoryCriterion() {
    return {this, m_criteria};
}

QQmlComponent* Category::delegate() const {
    return m_delegate;
}

void Category::setDelegate(QQmlComponent* d) {
    m_delegate = d;
    Q_EMIT delegateChanged(d);
}

CategoryProxyModel* Category::model() const {
    return m_category_model;
}

void Category::setModel(CategoryProxyModel* m) {
    m_category_model = m;
    Q_EMIT modelChanged(m);
}

Criterion::Criterion(QObject* parent) : QObject(parent) {
    connect(this, &Criterion::patternChanged, [this](QString p) { m_regex.setPattern(p); });
}

QString Criterion::pattern() const {
    return m_pattern;
}

void Criterion::setPattern(QString p) {
    m_pattern = p;
    Q_EMIT patternChanged(p);
}

void Criterion::setModel(CategoryProxyModel* model) {
    m_category_model = model;
    Q_EMIT modelChanged(m_category_model);
}

CategoryProxyModel* Criterion::model() const {
    return m_category_model;
}

Role::Role(QObject* parent) : Criterion(parent) {
}

QString Role::role() const {
    return m_role;
}

void Role::setRole(QString str) {
    m_role = str;
    Q_EMIT roleChanged(m_role);
}

QString Role::result(const QModelIndex& index) const {
    assert(index.isValid());
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
