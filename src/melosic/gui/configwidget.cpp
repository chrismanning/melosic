/**************************************************************************
**  Copyright (C) 2012 Christian Manning
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

#include <QLineEdit>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QVariant>

#include <melosic/common/stream.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/common/string.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/output.hpp>

#include "configwidget.hpp"

namespace {
QWidget* qVariantConfWidget(const QVariant& v);
}

namespace Melosic {

Logger::Logger logject(logging::keywords::channel = "ConfigWidget");

ConfigWidget::ConfigWidget(Config::Base& conf, QWidget* parent) :
    QWidget(parent), conf(conf)
{}

void ConfigWidget::restoreDefaults() {
    conf.resetToDefault();
    setup();
}

ConfigWidget* Config::Base::createWidget(QWidget* parent) {
    return new GenericConfigWidget(*this, parent);
}

QIcon* Config::Base::getIcon() const {
    return nullptr;
}

GenericConfigWidget::GenericConfigWidget(Config::Base& conf, QWidget* parent)
    : ConfigWidget(conf, parent)
{
    layout = new QVBoxLayout;
    setup();
    this->setLayout(layout);
}

void GenericConfigWidget::apply() {
    for(int i=0; i<form->rowCount(); ++i) {
        auto item = form->itemAt(i, QFormLayout::FieldRole);
        if(QWidget* w = item->widget()) {
            Config::VarType v;
            switch(ConfigType(w->property("type").toInt())) {
                case ConfigType::String:
                    if(auto le = dynamic_cast<QLineEdit*>(w))
                        v = le->text().toStdString();
                    break;
                case ConfigType::Bool:
                    if(auto cb = dynamic_cast<QCheckBox*>(w))
                        v = cb->isChecked();
                    break;
                case ConfigType::Int:
                    if(auto le = dynamic_cast<QLineEdit*>(w))
                        v = le->text().toLong();
                    break;
                case ConfigType::Float:
                    if(auto le = dynamic_cast<QLineEdit*>(w))
                        v = le->text().toDouble();
                    break;
                case ConfigType::Binary:
                    continue;
            }
            conf.putNode(w->property("key").toString().toStdString(), v);
        }
    }
}

void GenericConfigWidget::setup() {
    ConfigVisitor cv;
    if(layout->count()) {
        layout->removeWidget(gen);
        gen->deleteLater();
        form->deleteLater();
    }
    gen = new QGroupBox("General");
    form = new QFormLayout;
    for(const auto& node : conf.getNodes()) {
        QWidget* w = node.second.apply_visitor(cv);
        if(!w)
            continue;
        w->setProperty("key", QString::fromStdString(node.first));
        form->addRow(QString::fromStdString(toTitle(node.first)), w);
    }
    gen->setLayout(form);
    layout->insertWidget(0, gen);
}

std::vector<uint8_t> toVector(const QVariant& var) {
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);

    stream << var;

    return std::move(std::vector<uint8_t>(ba.cbegin(), ba.cend()));
}

QVariant fromVector(const std::vector<uint8_t>& vec) {
    QByteArray ba;
    ba.insert(0, reinterpret_cast<const char*>(vec.data()), vec.size());
    QDataStream stream(&ba, QIODevice::ReadOnly);

    QVariant var;
    stream >> var;

    return std::move(var);
}

QWidget* ConfigVisitor::operator()(const std::string& val) {
    auto le = new QLineEdit(QString::fromStdString(val));
    le->setProperty("type", int(ConfigType::String));
    return le;
}

QWidget* ConfigVisitor::operator()(bool val) {
    auto cb = new QCheckBox;
    cb->setChecked(val);
    cb->setProperty("type", int(ConfigType::Bool));
    return cb;
}

QWidget* ConfigVisitor::operator()(int64_t val) {
    auto spinner = new QSpinBox;
    spinner->setMaximum(std::numeric_limits<int>::max());
    spinner->setMinimum(std::numeric_limits<int>::min());
    spinner->setValue(static_cast<int>(val));
    spinner->setProperty("type", int(ConfigType::Int));
    return spinner;
}

QWidget* ConfigVisitor::operator()(double val) {
    auto spinner = new QDoubleSpinBox;
    spinner->setMaximum(std::numeric_limits<double>::max());
    spinner->setMinimum(std::numeric_limits<double>::min());
    spinner->setValue(val);
    spinner->setProperty("type", int(ConfigType::Float));
    return spinner;
}

QWidget* ConfigVisitor::operator()(const std::vector<uint8_t>& vec) {
    auto var = fromVector(vec);
    if(!var.isNull() && var.isValid())
        return qVariantConfWidget(var);
    return nullptr;
}

} // namespace Melosic

#include <QColor>
#include <QColorDialog>
#include <QDate>
#include <QDateTime>
#include <QFont>
#include <QPushButton>

namespace {

struct QVariantConfVisitor : Melosic::ConfigVisitor {
    QWidget* operator()(QColor) {
        return nullptr;
    }

    QWidget* operator()(int i) {
        return (*this)(static_cast<int64_t>(i));
    }

    template <typename T>
    QWidget* operator()(T&&) {
        return nullptr;
    }
};

QWidget* qVariantConfWidget(const QVariant& v) {
    QVariantConfVisitor cv;
    switch(v.userType()) {
        case QVariant::Color:
            return cv(v.value<QColor>());
        case QVariant::Date:
            return cv(v.value<QDate>());
        case QVariant::DateTime:
            return cv(v.value<QDateTime>());
        case QVariant::Font:
            return cv(v.value<QFont>());
        case QVariant::Icon:
            return cv(v.value<QIcon>());
        case QVariant::Image:
            return cv(v.value<QImage>());
        case QVariant::KeySequence:
            return cv(v.value<QKeySequence>());
        case QVariant::Line:
            return cv(v.value<QLine>());
        case QVariant::LineF:
            return cv(v.value<QLineF>());
        case QVariant::Locale:
            return cv(v.value<QLocale>());
        case QVariant::Transform:
            return cv(v.value<QTransform>());
        case QVariant::Pixmap:
            return cv(v.value<QPixmap>());
        case QVariant::Point:
            return cv(v.value<QPoint>());
        case QVariant::PointF:
            return cv(v.value<QPointF>());
        case QVariant::Polygon:
            return cv(v.value<QPolygon>());
        case QVariant::Rect:
            return cv(v.value<QRect>());
        case QVariant::RectF:
            return cv(v.value<QRectF>());
        case QVariant::RegExp:
            return cv(v.value<QRegExp>());
        case QVariant::Size:
            return cv(v.value<QSize>());
        case QVariant::SizeF:
            return cv(v.value<QSizeF>());
        case QVariant::SizePolicy:
            return cv(v.value<QSizePolicy>());
        case QVariant::TextFormat:
            return cv(v.value<QTextFormat>());
        case QVariant::Time:
            return cv(v.value<QTime>());
        case QVariant::String:
            return cv(v.toString().toStdString());
        case QVariant::Int:
            return cv(v.toInt());
        case QVariant::Double:
            return cv(v.toDouble());
        case QVariant::Bool:
            return cv(v.toBool());
        case QVariant::BitArray:
        case QVariant::Bitmap:
        case QVariant::Brush:
        case QVariant::ByteArray:
        case QVariant::Char:
        case QVariant::Cursor:
        case QVariant::EasingCurve:
        case QVariant::Hash:
        case QVariant::List:
        case QVariant::LongLong:
        case QVariant::Map:
        case QVariant::Matrix:
        case QVariant::Matrix4x4:
        case QVariant::Palette:
        case QVariant::Pen:
        case QVariant::Quaternion:
        case QVariant::Region:
        case QVariant::StringList:
        case QVariant::TextLength:
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QVariant::Url:
        case QVariant::Vector2D:
        case QVariant::Vector3D:
        case QVariant::Vector4D:
        case QVariant::UserType:
        case QVariant::Invalid:
        default:
            return nullptr;
    }
}

}
