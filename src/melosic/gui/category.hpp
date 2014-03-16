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

#ifndef MELOSIC_CATEGORY_HPP
#define MELOSIC_CATEGORY_HPP

#include <QObject>
#include <QQmlListProperty>
#include <QQmlComponent>
#include <QList>
#include <qqml.h>
#include <QRegularExpression>

namespace Melosic {

class CategoryProxyModel;
class Criterion;

class Category : public QObject {
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<Melosic::Criterion> categoryCriterion READ categoryCriterion FINAL)
    QList<Criterion*> m_criteria;

    Q_PROPERTY(CategoryProxyModel* model READ model WRITE setModel NOTIFY modelChanged)
    CategoryProxyModel* m_category_model{nullptr};
    friend class CategoryProxyModel;

    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    QQmlComponent* m_delegate;

    Q_CLASSINFO("DefaultProperty", "categoryCriterion")

public:
    explicit Category(QObject* parent = nullptr);

    QQmlListProperty<Criterion> categoryCriterion();

    QQmlComponent* delegate() const;
    void setDelegate(QQmlComponent* d);

    CategoryProxyModel* model() const;
    void setModel(CategoryProxyModel*);

Q_SIGNALS:
    void modelChanged(CategoryProxyModel* model);
    void delegateChanged(QQmlComponent* delegate);
};

class Criterion : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString pattern READ pattern WRITE setPattern NOTIFY patternChanged)
    QString m_pattern;
    friend class Category;

protected:
    CategoryProxyModel* m_category_model = nullptr;
    Q_PROPERTY(Melosic::CategoryProxyModel* model READ model WRITE setModel NOTIFY modelChanged)
    QRegularExpression m_regex;

public:
    explicit Criterion(QObject* parent = nullptr);
    virtual ~Criterion() {}

    QString pattern() const;
    void setPattern(QString p);

    void setModel(CategoryProxyModel* model);
    CategoryProxyModel* model() const;

    virtual QString result(const QModelIndex&) const = 0;

Q_SIGNALS:
    void patternChanged(QString pattern);
    void modelChanged(CategoryProxyModel* model);
};

class Role : public Criterion {
    Q_OBJECT
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged FINAL)
    QString m_role;

public:
    explicit Role(QObject* parent = nullptr);

    QString role() const;
    void setRole(QString str);

    virtual QString result(const QModelIndex&) const override;

Q_SIGNALS:
    void roleChanged(QString role);
};

} // namespace Melosic
QML_DECLARE_TYPE(Melosic::Category)
QML_DECLARE_TYPE(Melosic::Criterion)

#endif // MELOSIC_CATEGORY_HPP
