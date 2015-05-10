/**************************************************************************
**  Copyright (C) 2014 Christian Manning
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

#include <algorithm>
#include <numeric>
#include <type_traits>

#include <boost/core/null_deleter.hpp>
#include <boost/algorithm/clamp.hpp>

#include <QDialog>
#include <QDialogButtonBox>
#include <QStyle>
#include <QVBoxLayout>
#include <QLabel>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QSignalMapper>
#include <QComboBox>
#include <QPushButton>

#include <kpageview.h>
#include <kpagemodel.h>
#include <keditlistwidget.h>

#include <melosic/melin/config.hpp>
#include <melosic/common/int_get.hpp>

#include "configmanager.hpp"

namespace Melosic {

template <typename T> static constexpr int TypeIndex = boost::mpl::index_of<Config::VarType::types, T>::type::value;

template <typename Derived> struct ConfigWrapper {
    bool modified() const { return static_cast<const Derived*>(this)->modified(); }
    bool apply() { return static_cast<Derived*>(this)->apply(); }
    bool reset() { return static_cast<Derived*>(this)->reset(); }
    bool restore() { return static_cast<Derived*>(this)->restore(); }
};

template <typename FunT, typename ThisT>
static auto to_function(FunT&& fun, std::enable_if_t<std::is_member_function_pointer<FunT>::value, ThisT>* obj) {
    return std::mem_fn(fun, obj);
}

template <typename FunT, typename ThisT>
static auto to_function(FunT&& fun, std::enable_if_t<!std::is_member_function_pointer<FunT>::value, ThisT>*) {
    return std::forward<FunT>(fun);
}

template <typename WidgetT> struct ConfigWidget : Configurator {
    explicit ConfigWidget(WidgetT* widget) : Configurator(widget), m_widget(widget) { assert(m_widget); }

    template <typename ModifiedSignalT>
    explicit ConfigWidget(WidgetT* widget, ModifiedSignalT&& sig)
        : ConfigWidget(widget) {
        QObject::connect(widget, sig, [this](auto&&...) { Q_EMIT modifiedChanged(m_widget->modified()); });
    }

    bool modified() const override { return m_widget->modified(); }
    bool apply() override { return m_widget->apply(); }
    bool reset() override { return m_widget->reset(); }
    bool restore() override { return m_widget->restore(); }

    WidgetT* m_widget;
};

struct GeneratorConfWidget : QWidget, Configurator {
    virtual ~GeneratorConfWidget() {}

    bool modified() const override {
        for(auto&& child : QWidget::children()) {
            if(auto obj = dynamic_cast<Configurable*>(child)) {
                assert(obj->configure());
                if(obj->configure()->modified())
                    return true;
            }
        }
        return false;
    }

    bool apply() override {
        for(auto&& child : QWidget::children()) {
            if(auto obj = dynamic_cast<Configurable*>(child)) {
                assert(obj->configure());
                obj->configure()->apply();
            }
        }
        return false;
    }

    bool reset() override {
        for(auto&& child : QWidget::children()) {
            if(auto obj = dynamic_cast<Configurable*>(child)) {
                assert(obj->configure());
                obj->configure()->reset();
            }
        }
        return false;
    }

    bool restore() override {
        for(auto&& child : QWidget::children()) {
            if(auto obj = dynamic_cast<Configurable*>(child)) {
                assert(obj->configure());
                obj->configure()->restore();
            }
        }
        return false;
    }
};

struct ConfigLineEdit : QLineEdit, Configurable {
    ConfigLineEdit(const std::shared_ptr<Config::Conf>& c, const std::string& name, const std::string& str)
        : QLineEdit(QString::fromStdString(str)), m_conf(c), m_name(name), m_reset_value(str),
          m_configurator(new ConfigWidget<ConfigLineEdit>(this)) {
        m_sig_conn = connect(this, &QLineEdit::textChanged, [this](auto&&) {
            disconnect(m_sig_conn);
            Q_EMIT m_configurator->modifiedChanged(QLineEdit::isModified());
        });
    }

    bool modified() const { return QLineEdit::isModified(); }

    bool apply() {
        if(!modified())
            return false;
        m_reset_value = text().toStdString();
        m_conf->putNode(m_name, m_reset_value);

        return reset();
    }

    bool reset() {
        if(!modified())
            return false;
        setText(QString::fromStdString(m_reset_value));
        assert(!isModified());
        Q_EMIT m_configurator->modifiedChanged(isModified());
        m_sig_conn = connect(this, &QLineEdit::textChanged, [this](auto&&) {
            disconnect(m_sig_conn);
            Q_EMIT m_configurator->modifiedChanged(QLineEdit::isModified());
        });
        return true;
    }

    bool restore() { return false; }

    Configurator* configure() const override { return m_configurator; }

    const std::shared_ptr<Config::Conf> m_conf;
    const std::string m_name;
    std::string m_reset_value;
    ConfigWidget<ConfigLineEdit>* m_configurator{nullptr};
    QMetaObject::Connection m_sig_conn;
};

struct ConfigCheckBox : QCheckBox, Configurable {
    ConfigCheckBox(const std::shared_ptr<Config::Conf>& c, const std::string& name, bool val)
        : QCheckBox(QString::fromStdString(name)), m_conf(c), m_name(name), m_reset_value(val),
          m_configurator(new ConfigWidget<ConfigCheckBox>(this, &QCheckBox::toggled)) {
        setChecked(val);
    }

    bool modified() const { return isChecked() != m_reset_value; }

    bool apply() {
        if(!modified())
            return false;
        m_reset_value = isChecked();
        m_conf->putNode(m_name, m_reset_value);

        return reset();
    }

    bool reset() {
        if(!modified())
            return false;
        setChecked(m_reset_value);
        return true;
    }

    bool restore() { return false; }

    Configurator* configure() const override { return m_configurator; }

    const std::shared_ptr<Config::Conf> m_conf;
    const std::string m_name;
    bool m_reset_value;
    ConfigWidget<ConfigCheckBox>* m_configurator{nullptr};
};

struct ConfigSpinBox : QSpinBox, Configurable {
    ConfigSpinBox(const std::shared_ptr<Config::Conf>& c, const std::string& name, double val)
        : QSpinBox(), m_conf(c), m_name(name), m_reset_value(val),
          m_configurator(new ConfigWidget<ConfigSpinBox>(this)) {
        setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
        setValue(val);
        connect(this, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int val) {
            if(val != m_reset_value && !m_modified)
                Q_EMIT m_configurator->modifiedChanged(true);
            else if(val == m_reset_value && m_modified)
                Q_EMIT m_configurator->modifiedChanged(false);
            m_modified = val != m_reset_value;
        });
    }

    bool modified() const { return value() != m_reset_value; }

    bool apply() {
        if(!modified())
            return false;
        m_reset_value = value();
        m_conf->putNode(m_name, m_reset_value);

        return reset();
    }

    bool reset() {
        if(!modified())
            return false;
        setValue(m_reset_value);
        return true;
    }

    bool restore() { return false; }

    Configurator* configure() const override { return m_configurator; }

    const std::shared_ptr<Config::Conf> m_conf;
    const std::string m_name;
    int m_reset_value;
    bool m_modified{false};
    ConfigWidget<ConfigSpinBox>* m_configurator{nullptr};
};

struct ConfigDoubleSpinBox : QDoubleSpinBox, Configurable {
    ConfigDoubleSpinBox(const std::shared_ptr<Config::Conf>& c, const std::string& name, double val)
        : QDoubleSpinBox(), m_conf(c), m_name(name), m_reset_value(val),
          m_configurator(new ConfigWidget<ConfigDoubleSpinBox>(this)) {
        setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
        setValue(val);
        connect(this, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](double val) {
            if(val != m_reset_value && !m_modified)
                Q_EMIT m_configurator->modifiedChanged(true);
            else if(val == m_reset_value && m_modified)
                Q_EMIT m_configurator->modifiedChanged(false);
            m_modified = val != m_reset_value;
        });
    }

    bool modified() const { return value() != m_reset_value; }

    bool apply() {
        if(!modified())
            return false;
        m_reset_value = value();
        m_conf->putNode(m_name, m_reset_value);

        return reset();
    }

    bool reset() {
        if(!modified())
            return false;
        setValue(m_reset_value);
        return true;
    }

    bool restore() { return false; }

    Configurator* configure() const override { return m_configurator; }

    const std::shared_ptr<Config::Conf> m_conf;
    const std::string m_name;
    double m_reset_value;
    bool m_modified{false};
    ConfigWidget<ConfigDoubleSpinBox>* m_configurator{nullptr};
};

struct ConfigEditList : KEditListWidget, Configurable {
    ConfigEditList(const std::shared_ptr<Config::Conf>& c, const std::string& name,
                   const std::vector<Config::VarType>& val)
        : KEditListWidget(), m_conf(c), m_name(name), m_reset_value(val),
          m_configurator(new ConfigWidget<ConfigEditList>(this)) {
        setItems(val);
        connect(this, &KEditListWidget::changed, [this]() {
            if(!m_modified) {
                m_modified = true;
                Q_EMIT m_configurator->modifiedChanged(m_modified);
            }
        });
    }

    using KEditListWidget::setItems;
    void setItems(const std::vector<Config::VarType>& items) {
        using boost::get;
        QStringList q_items;
        for(auto&& item : items) {
            if(item.which() == TypeIndex<std::string>)
                q_items.push_back(QString::fromStdString(get<std::string>(item)));
            else if(item.which() == TypeIndex<double>)
                q_items.push_back(QString::number(get<double>(item)));
            else if(item.which() == TypeIndex<std::vector<Config::VarType>>) {
            } else
                q_items.push_back(QString::number(get<int>(item)));
        }
        setItems(q_items);
    }

    bool modified() const { return m_modified; }

    bool apply() {
        if(!modified())
            return false;
        decltype(m_reset_value) new_value;
        for(auto&& item : items())
            new_value.emplace_back(item.toStdString());

        m_conf->putNode(m_name, new_value);
        new_value.swap(m_reset_value);

        return reset();
    }

    bool reset() {
        if(!modified())
            return false;
        setItems(m_reset_value);
        m_modified = false;
        Q_EMIT m_configurator->modifiedChanged(m_modified);
        return true;
    }

    bool restore() { return false; }

    Configurator* configure() const override { return m_configurator; }

    const std::shared_ptr<Config::Conf> m_conf;
    const std::string m_name;
    std::vector<Config::VarType> m_reset_value;
    bool m_modified{false};
    ConfigWidget<ConfigEditList>* m_configurator{nullptr};
};

static GeneratorConfWidget* generate_config_widget(std::shared_ptr<Config::Conf>);

struct WidgetGenerator : boost::static_visitor<Configurator*> {
    explicit WidgetGenerator(const std::shared_ptr<Config::Conf>& c, QFormLayout* l, const std::string& name)
        : m_conf(c), m_layout(l), m_name(name) {
        auto qname = QString::fromStdString(m_name);
        m_label = new QLabel(qname + ':');

        auto font = m_label->font();
        font.setCapitalization(QFont::Capitalize);
        m_label->setFont(font);
    }

    Configurator* operator()(const std::string& str) {
        auto le = new ConfigLineEdit(m_conf, m_name, str);
        static_assert(std::is_base_of<QWidget, ConfigLineEdit>::value, "");
        static_assert(std::is_base_of<Configurable, ConfigLineEdit>::value, "");
        m_layout->addRow(m_label, le);
        return le->configure();
    }

    Configurator* operator()(bool val) {
        auto cb = new ConfigCheckBox(m_conf, m_name, val);

        auto font = cb->font();
        font.setCapitalization(QFont::Capitalize);
        cb->setFont(font);

        m_layout->addRow(cb);
        return cb->configure();
    }

    template <typename T>
    std::enable_if_t<std::is_integral<T>::value && !std::is_same<bool, T>::value, Configurator>* operator()(T val_) {
        int val = boost::algorithm::clamp(val_, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        auto sb = new ConfigSpinBox(m_conf, m_name, val);

        m_layout->addRow(m_label, sb);
        return sb->configure();
    }

    Configurator* operator()(double val) {
        auto dbl_sb = new ConfigDoubleSpinBox(m_conf, m_name, val);

        m_layout->addRow(m_label, dbl_sb);
        return dbl_sb->configure();
    }

    Configurator* operator()(const std::vector<Config::VarType>& val) {
        auto edit_list = new ConfigEditList(m_conf, m_name, val);

        m_layout->addRow(m_label, edit_list);
        return edit_list->configure();
    }

    std::shared_ptr<Config::Conf> m_conf;
    QFormLayout* m_layout;
    const std::string& m_name;
    QLabel* m_label;
};

static GeneratorConfWidget* generate_config_widget(std::shared_ptr<Config::Conf> conf) {
    auto conf_widget = new GeneratorConfWidget;

    auto layout = new QFormLayout(conf_widget);

    auto signal_mapper = new QSignalMapper(static_cast<Configurator*>(conf_widget));
    QObject::connect(signal_mapper, static_cast<void (QSignalMapper::*)(QObject*)>(&QSignalMapper::mapped),
                     [conf_widget](QObject*) -> void { Q_EMIT conf_widget->modifiedChanged(conf_widget->modified()); });

    conf->iterateNodes([=](auto&& name, auto&& var) {
        WidgetGenerator gen(conf, layout, name);
        auto ret = var.apply_visitor(gen);
        if(!ret)
            return;
        assert(ret);
        QObject::connect(ret, &Configurator::modifiedChanged, [signal_mapper, ret](auto&&) {
            std::clog << "some widget modified: " << std::boolalpha << ret->modified() << std::endl;
            Q_EMIT signal_mapper->map(static_cast<Configurator*>(ret));
        });
        signal_mapper->setMapping(ret, ret);
    });

    return conf_widget;
}

struct ConfItem;

struct ChildConfigurator : Configurator {
    ChildConfigurator(ConfItem&);

    bool modified() const override;
    bool apply() override;
    bool reset() override;
    bool restore() override;

    ConfItem& m_root;
};

template <typename FunT> static void traverse_pre_order(ConfItem&, FunT&&);
template <typename FunT> static void traverse_post_order(ConfItem&, FunT&&);

struct ConfItem {
    explicit ConfItem(std::shared_ptr<Config::Conf> c, ConfItem* parent = nullptr) : m_conf(c), m_parent(parent) {
        if(!c)
            return;
        m_children.reserve(c->childCount());
        c->iterateChildren([this](auto&& child) {
            assert(child);
            m_children.emplace_back(std::const_pointer_cast<Config::Conf>(child), this);
        });

        if(c->nodeCount() == 0)
            return;

        auto cw = generate_config_widget(c);

        auto scroll_area = new QScrollArea;
        scroll_area->setWidget(cw);
        scroll_area->setWidgetResizable(true);

        m_widget = scroll_area;
        m_configurable = cw;
    }

    explicit ConfItem(Config::Conf* c, ConfItem* parent = nullptr) : ConfItem({c, boost::null_deleter{}}, parent) {}

    ConfItem(ConfItem&& b) noexcept { b.swap(*this); }
    ConfItem& operator=(ConfItem&&) = default;

    ConfItem(const ConfItem&) = delete;
    ConfItem& operator=(const ConfItem&) = delete;

    ~ConfItem() { delete m_widget; }

    QWidget* widget() const { return m_widget; }

    Configurator* configure() const { return m_configurable; }

    void swap(ConfItem& b) noexcept {
        using std::swap;
        swap(m_children, b.m_children);
        swap(m_nodes, b.m_nodes);
        swap(m_conf, b.m_conf);
        swap(m_parent, b.m_parent);
        swap(m_widget, b.m_widget);
        swap(m_configurable, b.m_configurable);
        swap(m_sig_map_conn, b.m_sig_map_conn);
    }

    std::vector<ConfItem> m_children;
    std::vector<std::pair<QString, QVariant>> m_nodes;
    std::shared_ptr<Config::Conf> m_conf;
    ConfItem* m_parent{nullptr};
    QWidget* m_widget{nullptr};
    Configurator* m_configurable{nullptr};
    QMetaObject::Connection m_sig_map_conn;
};

template <typename FunT> static void traverse_pre_order(ConfItem& root, FunT&& fun) {
    fun(root);
    for(auto&& child : root.m_children)
        traverse_pre_order(child, fun);
}

template <typename FunT> static void traverse_post_order(ConfItem& root, FunT&& fun) {
    for(auto&& child : root.m_children)
        traverse_post_order(child, fun);
    fun(root);
}

ChildConfigurator::ChildConfigurator(ConfItem& c) : m_root(c) {}

bool ChildConfigurator::modified() const { return false; }

bool ChildConfigurator::apply() {
    traverse_pre_order(m_root, [this](auto&& child) {
        assert(child.configure());
        if(child.configure() == this)
            return;
        child.configure() && child.configure()->apply();
    });
    return true;
}

bool ChildConfigurator::reset() {
    traverse_pre_order(m_root, [this](auto&& child) {
        assert(child.configure());
        if(child.configure() == this)
            return;
        child.configure() && child.configure()->reset();
    });
    return true;
}

bool ChildConfigurator::restore() {
    traverse_pre_order(m_root, [this](auto&& child) {
        assert(child.configure());
        if(child.configure() == this)
            return;
        child.configure() && child.configure()->restore();
    });
    return true;
}

struct ConfTreeModel : KPageModel {
    explicit ConfTreeModel(Config::Conf& c, QObject* parent = nullptr) : m_config_root(&c) {
        assert(!m_config_root.m_configurable);
        if(!m_config_root.m_configurable)
            m_config_root.m_configurable = new ChildConfigurator(m_config_root);
    }

    virtual ~ConfTreeModel() {
        assert(m_config_root.m_configurable);
        if(m_config_root.m_configurable && dynamic_cast<ChildConfigurator*>(m_config_root.m_configurable))
            delete m_config_root.m_configurable;
    }

    enum ConfigRoles { ModifiedRole = Qt::UserRole * 7 };

    void setRootConf(Config::Conf& c) {
        beginResetModel();
        m_config_root = ConfItem{&c, nullptr};
        endResetModel();
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override {
        auto parentItem = getItem(parent);
        if(!parentItem || row < 0 || parentItem->m_children.size() < static_cast<size_t>(row))
            return {};

        return createIndex(row, column, &parentItem->m_children[+row]);
    }

    QModelIndex parent(const QModelIndex& index) const override {
        auto item = getItem(index);
        if(!item || !item->m_parent)
            return {};
        int row{0};
        if(auto parent = item->m_parent->m_parent) {
            auto it = std::find_if(parent->m_children.begin(), parent->m_children.end(),
                                   [item](auto&& var) { return &var == item->m_parent; });
            row = std::distance(parent->m_children.begin(), it);
        }
        return createIndex(row, 0, item->m_parent);
    }

    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override {
        auto item = getItem(parent);
        return item && !item->m_children.empty();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        auto item = getItem(parent);
        return item ? item->m_children.size() : 0;
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if(!index.isValid())
            return {};

        auto item = getItem(index);
        if(!item || item == &m_config_root)
            return {};

        if(role == Qt::DisplayRole) {
            assert(item->configure());
            return QString::fromStdString(item->m_conf->name()) +
                   (item->configure() && item->configure()->modified() ? "*" : "");
        } else if(role == ModifiedRole && item->configure()) {
            return QVariant{item->configure()->modified()};
        } else if(role == KPageModel::WidgetRole && item->m_widget) {
            return QVariant::fromValue(static_cast<QWidget*>(item->m_widget));
        } else if(role == KPageModel::HeaderRole) {
            return QString::fromStdString(item->m_conf->name());
        }
        return {};
    }

    int columnCount(const QModelIndex& = QModelIndex()) const override { return 1; }

    ConfItem* getItem(const QModelIndex& index) const {
        if(!index.isValid())
            return &const_cast<ConfTreeModel*>(this)->m_config_root;
        ConfItem* item = static_cast<ConfItem*>(index.internalPointer());
        assert(item);
        if(!item)
            return nullptr;

        if(item->configure() && !item->m_sig_map_conn) {
            item->m_sig_map_conn =
                connect(item->configure(), &Configurator::modifiedChanged, [this, index, item](auto&&) {
                    Q_EMIT const_cast<ConfTreeModel*>(this)->dataChanged(index, index, {ModifiedRole, Qt::DisplayRole});
                });
        }
        return item;
    }

    ConfItem m_config_root;
};

struct ConfigManager::impl {
    void onConfigManagerChanged() {
        assert(m_confman);
        if(!m_confman)
            return;

        m_window = new QDialog;
        m_window->setWindowTitle("Configuration[*]");
        m_window->setWindowModality(Qt::ApplicationModal);

        m_window->setMinimumSize({400, 400});
        m_window->resize({400, 400});

        auto layout = new QVBoxLayout;

        auto pages = new KPageView;
        pages->setFaceType(KPageView::Tree);

        auto conf = m_confman->getConfigRoot().synchronize();
        auto model = new ConfTreeModel(*conf);
        pages->setModel(model);
        pages->setDefaultWidget(new QLabel("no widget"));

        layout->addWidget(pages);

        auto buttons = new QDialogButtonBox;
        buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply |
                                    QDialogButtonBox::Reset | QDialogButtonBox::RestoreDefaults);

        QObject::connect(buttons, &QDialogButtonBox::clicked, [buttons, this, model](auto&& button) {
            assert(model->m_config_root.configure());
            if(button == static_cast<QAbstractButton*>(buttons->button(QDialogButtonBox::Ok))) {
                std::clog << "Ok clicked\n";
                model->m_config_root.configure()->apply();
                m_confman->saveConfig();
                m_window->hide();
            } else if(button == static_cast<QAbstractButton*>(buttons->button(QDialogButtonBox::Cancel))) {
                std::clog << "Cancel clicked\n";
                model->m_config_root.configure()->reset();
                m_window->hide();
            } else if(button == static_cast<QAbstractButton*>(buttons->button(QDialogButtonBox::Apply))) {
                std::clog << "Apply clicked\n";
                model->m_config_root.configure()->apply();
                m_confman->saveConfig();
            } else if(button == static_cast<QAbstractButton*>(buttons->button(QDialogButtonBox::Reset))) {
                std::clog << "Reset clicked\n";
                model->m_config_root.configure()->reset();
            } else if(button == static_cast<QAbstractButton*>(buttons->button(QDialogButtonBox::RestoreDefaults))) {
                std::clog << "Restore Defaults clicked\n";
            } else
                assert(false);
        });

        layout->addWidget(buttons);

        m_window->setLayout(layout);

        if(buttons->width() > m_window->minimumWidth())
            m_window->setMinimumWidth(buttons->width());
    }

    std::shared_ptr<Config::Manager> m_confman{nullptr};
    QDialog* m_window{nullptr};
    QWindow* m_parent{nullptr};
};

ConfigManager::ConfigManager() : pimpl(std::make_unique<impl>()) {
    connect(this, &ConfigManager::configManagerChanged, [this]() { pimpl->onConfigManagerChanged(); });
}

ConfigManager::~ConfigManager() {}

ConfigManager* ConfigManager::instance() {
    static ConfigManager confman;
    return &confman;
}

const std::shared_ptr<Config::Manager>& Melosic::ConfigManager::getConfigManager() const { return pimpl->m_confman; }

void ConfigManager::setConfigManager(const std::shared_ptr<Config::Manager>& confman) {
    pimpl->m_confman = confman;
    Q_EMIT configManagerChanged();
}

void ConfigManager::openWindow() {
    assert(pimpl->m_window);
    if(!pimpl->m_window)
        return;
    pimpl->m_window->setGeometry(
        QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, pimpl->m_window->size(), pimpl->m_parent->geometry()));
    pimpl->m_window->showNormal();
}

QWindow* ConfigManager::getParent() const { return pimpl->m_parent; }

void ConfigManager::setParent(QWindow* parent) { pimpl->m_parent = parent; }

} // namespace Melosic
