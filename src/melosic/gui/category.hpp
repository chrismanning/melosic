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
#include <QList>
#include <QQmlComponent>
#include <qqml.h>

namespace Melosic {

class CategoryProxyModel;
class Criteria;

class Category : public QObject {
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<Melosic::Criteria> categoryCriteria READ categoryCriteria FINAL)
    QList<Criteria*> criteria_;

    Q_PROPERTY(Melosic::CategoryProxyModel* model READ model WRITE setModel NOTIFY modelChanged FINAL)
    CategoryProxyModel* model_;
    friend class CategoryProxyModel;

    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    QQmlComponent* delegate_;

    Q_CLASSINFO("DefaultProperty", "categoryCriteria")

public:
    explicit Category(QObject* parent = nullptr);

    QQmlListProperty<Criteria> categoryCriteria();

    CategoryProxyModel* model() const;
    void setModel(CategoryProxyModel* m);

    QQmlComponent* delegate() const;
    void setDelegate(QQmlComponent* d);

Q_SIGNALS:
    void modelChanged(CategoryProxyModel* model);
    void delegateChanged(QQmlComponent* delegate);
};

class Criteria : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant pattern READ pattern WRITE setPattern NOTIFY patternChanged FINAL)
    QVariant pattern_;
    friend class Category;

protected:
    CategoryProxyModel* model = nullptr;

public:
    explicit Criteria(QObject* parent = nullptr);
    virtual ~Criteria() {}

    QVariant pattern() const;
    void setPattern(QVariant p);

    virtual QString result(const QModelIndex&) const = 0;

Q_SIGNALS:
    void patternChanged(QVariant pattern);
};

class Role : public Criteria {
    Q_OBJECT
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged FINAL)
    QString role_;

public:
    explicit Role(QObject* parent = nullptr);

    QString role() const;
    void setRole(QString str);

    virtual QString result(const QModelIndex&) const override;

Q_SIGNALS:
    void roleChanged(QString role);
};

class Tag : public Criteria {
    Q_OBJECT
    Q_PROPERTY(QString field READ field WRITE setField NOTIFY fieldChanged FINAL)
    QString field_;

public:
    explicit Tag(QObject* parent = nullptr);

    QString field() const;
    void setField(QString str);

    virtual QString result(const QModelIndex&) const override;

Q_SIGNALS:
    void fieldChanged(QString field);
};

} // namespace Melosic
QML_DECLARE_TYPE(Melosic::Category)
QML_DECLARE_TYPE(Melosic::Criteria)
QML_DECLARE_TYPE(Melosic::Tag)

#endif // MELOSIC_CATEGORY_HPP
