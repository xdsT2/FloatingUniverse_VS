#include <QHBoxLayout>
#include <QLabel>
#include <QStyleOption>
#include <QInputDialog>
#include <QColorDialog>
#include <QDesktopServices>
#include "settingsitemlistbox.h"
#include "usettings.h"
#include "sapid_switch/boundaryswitchbase.h"
#include "sapid_switch/lovelyheartswitch.h"
#include "sapid_switch/normalswitch.h"
#include "anicirclelabel.h"
#include "aninumberlabel.h"

SettingsItemListBox::SettingsItemListBox(QWidget *parent) : QWidget(parent)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
}

void SettingsItemListBox::add(QPixmap pixmap, QString text, QString desc)
{
    createBg(pixmap, text, desc);
}

void SettingsItemListBox::add(QPixmap pixmap, QString text, QString desc, QString key, bool *val)
{
    auto btn = createBg(pixmap, text, desc);
    auto layout = static_cast<QHBoxLayout*>(btn->layout());
    auto swtch = new NormalSwitch(*val, btn);
    swtch->setSuitableWidth(us->widgetSize);
    layout->addWidget(swtch);

    auto changed = [=](bool v){
        us->set(key, *val = v);
    };
    connect(btn, &InteractiveButtonBase::clicked, btn, [=]{
        changed(!*val);
        swtch->setState(*val);
    });
    connect(swtch, &SapidSwitchBase::stateChanged, btn, [=](bool v){ changed(v); });
}

void SettingsItemListBox::add(QPixmap pixmap, QString text, QString desc, QString key, int *val, int min, int max, int step)
{
    auto btn = createBg(pixmap, text, desc);
    auto layout = static_cast<QHBoxLayout*>(btn->layout());
    auto label = new AniNumberLabel(*val, btn);
    label->setAlignment(Qt::AlignCenter);
    label->setFixedWidth(us->widgetSize);
    layout->addWidget(label);

    auto changed = [=](int v) {
        us->set(key, *val = v);
    };
    connect(btn, &InteractiveButtonBase::clicked, btn, [=]{
        bool ok = false;
        int x = QInputDialog::getInt(this, desc.isEmpty() ? "输入数值" : text,
                                     desc.isEmpty() ? text : desc,
                                     *val, min, max, step, &ok);
        if (!ok)
            return ;
        changed(x);
        label->setShowNum(x);
    });
}

void SettingsItemListBox::add(QPixmap pixmap, QString text, QString desc, QString key, double *val, double min, double max, double step)
{
    auto btn = createBg(pixmap, text, desc);
    auto layout = static_cast<QHBoxLayout*>(btn->layout());
    auto label = new AniNumberLabel(btn);
    label->setNum(*val);
    label->setAlignment(Qt::AlignCenter);
    label->setFixedWidth(us->widgetSize);
    layout->addWidget(label);

    auto changed = [=](double v) {
        us->set(key, *val = v);
    };
    connect(btn, &InteractiveButtonBase::clicked, btn, [=]{
        bool ok = false;
        double x = QInputDialog::getDouble(this, desc.isEmpty() ? "输入数值" : text,
                                           desc.isEmpty() ? text : desc,
                                           *val, min, max, 2, &ok);
        if (!ok)
            return ;
        changed(x);
        label->setNum(x);
    });
}

void SettingsItemListBox::add(QPixmap pixmap, QString text, QString desc, QString key, QString *val)
{
    auto btn = createBg(pixmap, text, desc);
    auto layout = static_cast<QHBoxLayout*>(btn->layout());
    QLabel* label = new QLabel(*val, btn);
    layout->addWidget(label);

    auto changed = [=](QString v) {
        us->set(key, *val = v);
    };
    connect(btn, &InteractiveButtonBase::clicked, btn, [=]{
        bool ok = false;
        QString s = QInputDialog::getText(this, desc.isEmpty() ? "输入文字" : text,
                                     desc.isEmpty() ? text : desc, QLineEdit::Normal, *val, &ok);
        if (!ok)
            return ;
        changed(s);
        label->setText(s);
    });
}

void SettingsItemListBox::add(QPixmap pixmap, QString text, QString desc, QString key, QColor *val)
{
    auto btn = createBg(pixmap, text, desc);
    auto layout = static_cast<QHBoxLayout*>(btn->layout());
    auto label = new AniCircleLabel(*val, btn);
    layout->addWidget(label);
    label->setFixedWidth(us->widgetSize);

    auto changed = [=](QColor v) {
        us->set(key, *val = v);
    };
    connect(btn, &InteractiveButtonBase::clicked, btn, [=]{
        QColor c = QColorDialog::getColor(*val, this, text, QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        changed(c);
        label->setMainColor(c);
    });
}

void SettingsItemListBox::addOpen(QPixmap pixmap, QString text, QString desc, QString payload)
{
    auto btn = createBg(pixmap, text, desc);
    auto layout = static_cast<QHBoxLayout*>(btn->layout());
    auto label = new QLabel(btn);
    label->setPixmap(QPixmap(":/icons/sub_menu_arrow"));
    label->setScaledContents(true);
    label->setFixedSize(20, 20);
    layout->addWidget(label);
    connect(btn, &InteractiveButtonBase::clicked, btn, [=]{
        emit openPage(payload);
    });
}

void SettingsItemListBox::addOpen(QPixmap pixmap, QString text, QString desc, QUrl url)
{
    auto btn = createBg(pixmap, text, desc);
    auto layout = static_cast<QHBoxLayout*>(btn->layout());
    auto label = new QLabel(btn);
    label->setPixmap(QPixmap(":/icons/st/open_url.png"));
    label->setScaledContents(true);
    label->setFixedSize(20, 20);
    layout->addWidget(label);
    connect(btn, &InteractiveButtonBase::clicked, btn, [=]{
        QDesktopServices::openUrl(url);
    });
}

void SettingsItemListBox::addPage(QPixmap pixmap, QString text, QString desc)
{
    auto btn = createBg(pixmap, text, desc);
    auto layout = static_cast<QHBoxLayout*>(btn->layout());
    auto label = new QLabel(btn);
    label->setPixmap(QPixmap(":/icons/sub_menu_arrow"));
    label->setScaledContents(true);
    label->setFixedSize(20, 20);
    layout->addWidget(label);
}

InteractiveButtonBase *SettingsItemListBox::lastItem() const
{
    return _lastItem;
}

int SettingsItemListBox::setFind(QString key)
{
    const QColor normalColor = Qt::transparent;
    const QColor selectColor = Qt::lightGray;
    if (key.isEmpty())
    {
        foreach (auto item, items)
        {
            item->setNormalColor(normalColor);
        }
        return -1;
    }

    int index = -1;
    for (int i = 0; i < texts.size(); i++)
    {
        if (texts.at(i).contains(key))
        {
            if (index == -1)
                index = i;
            items.at(i)->setNormalColor(selectColor);
        }
        else
        {
            items.at(i)->setNormalColor(normalColor);
        }
    }

    return index;
}

InteractiveButtonBase *SettingsItemListBox::createBg(QPixmap pixmap, QString text, QString desc)
{
    InteractiveButtonBase* btn = new InteractiveButtonBase(this);

    btn->setCursor(Qt::PointingHandCursor);

    bool isDark = isDarkTheme();

    const int iconSpacing = 16;
    auto hlayout = new QHBoxLayout(btn);
    hlayout->setContentsMargins(12, 12, 12, 12);
    hlayout->setSpacing(iconSpacing);
    hlayout->addSpacing(iconSpacing - hlayout->contentsMargins().top() + 2);

    QLabel* titleLabel = new QLabel(text, btn);
    titleLabel->adjustSize();
    titleLabel->setStyleSheet(isDark ? "color: #e0e0e0;" : "color: #202020;");
    int sz = titleLabel->height();
    int hintHeight = sz;

    if (!pixmap.isNull())
    {
        QLabel* label = new QLabel(btn);
        label->setScaledContents(true);
        label->setPixmap(pixmap);
        label->setFixedSize(sz, sz);
        hlayout->addWidget(label);
    }

    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->addWidget(titleLabel);
    vlayout->setSpacing(0);
    if (!desc.isEmpty())
    {
        QLabel* descLabel = new QLabel(desc, btn);
        vlayout->addWidget(descLabel);
        descLabel->setStyleSheet(isDark ? "color: #aaa;" : "color: #666");
        descLabel->adjustSize();
        hintHeight += descLabel->height() + vlayout->spacing();
    }
    hlayout->addLayout(vlayout);
    hlayout->setStretch(hlayout->count() - 1, 1);
    btn->setFixedHeight(hintHeight += hlayout->contentsMargins().top() * 2);

    if (items.size())
    {
        // 添加分割线
        QWidget* w = new QWidget(this);
        w->setFixedHeight(1);
        w->setStyleSheet(isDark ? "background-color: #444444;" : "background-color: #20888888");
        mainLayout->addWidget(w);
        w->setCursor(Qt::PointingHandCursor);
    }
    mainLayout->addWidget(btn);
    _lastItem = btn;
    items.append(btn);
    texts.append(text + "\n" + desc);
    return btn;
}

void SettingsItemListBox::adjusItemsSize()
{

}

void SettingsItemListBox::updateTheme()
{
    bool isDark = isDarkTheme();

    // 遍历所有已有的按钮项，更新文字颜色
    for (auto item : items)
    {
        if (!item) continue;
        auto labels = item->findChildren<QLabel*>();
        if (labels.isEmpty()) continue;
        // 第一个 QLabel 是标题
        labels.first()->setStyleSheet(isDark ? "color: #e0e0e0;" : "color: #202020;");
        // 其余是描述
        for (int i = 1; i < labels.size(); i++)
            labels.at(i)->setStyleSheet(isDark ? "color: #aaa;" : "color: #666;");
    }
    
    // 更新主题切换按钮的样式（如果存在）
    auto toggleGroups = this->findChildren<QWidget*>();
    for (auto group : toggleGroups) {
        if (group->objectName().startsWith("ThemeToggle")) {
            group->setStyleSheet(isDark
                ? "background: #2a2a2a; border: 1px solid #555; border-radius: 16px;"
                : "background: #f0f0f0; border: 1px solid #ddd; border-radius: 16px;"
            );
            // 更新三个按钮
            for (int j = 0; j < 3; j++) {
                QPushButton* b = group->findChild<QPushButton*>(QString("ThemeOpt%1").arg(j));
                if (b) {
                    if (us->themeMode == j) {
                        b->setStyleSheet(
                            "background: #3367D6; color: white; border: none; border-radius: 14px; font-size: 12px; font-weight: bold;"
                        );
                    } else {
                        b->setStyleSheet(
                            QString("background: transparent; color: %1; border: none; border-radius: 14px; font-size: 12px;")
                                .arg(isDark ? "#aaa" : "#888")
                        );
                    }
                }
            }
        }
    }
    
    update();
}

InteractiveButtonBase* SettingsItemListBox::addThreeStateTheme()
{
    InteractiveButtonBase* btn = new InteractiveButtonBase(this);
    btn->setCursor(Qt::PointingHandCursor);

    bool isDark = isDarkTheme();

    auto hlayout = new QHBoxLayout(btn);
    hlayout->setContentsMargins(12, 10, 12, 10);
    hlayout->setSpacing(12);

    // 标题
    QLabel* titleLabel = new QLabel("主题颜色", btn);
    titleLabel->setStyleSheet(isDark ? "color: #e0e0e0; font-weight: bold;" : "color: #202020; font-weight: bold;");
    titleLabel->adjustSize();
    hlayout->addSpacing(12);
    hlayout->addWidget(titleLabel);

    hlayout->addStretch();

    // 三段式选择按钮组（圆角容器）
    QWidget* toggleGroup = new QWidget(btn);
    toggleGroup->setObjectName("ThemeToggleGroup");
    toggleGroup->setFixedSize(190, 32);
    toggleGroup->setStyleSheet(isDark
        ? "background: #2a2a2a; border: 1px solid #555; border-radius: 16px;"
        : "background: #f0f0f0; border: 1px solid #ddd; border-radius: 16px;"
    );

    QHBoxLayout* toggleLayout = new QHBoxLayout(toggleGroup);
    toggleLayout->setContentsMargins(2, 2, 2, 2);
    toggleLayout->setSpacing(2);

    const char* themeLabels[] = {"浅色", "深色", "跟随"};
    QPushButton* optBtns[3] = {nullptr, nullptr, nullptr};

    for (int i = 0; i < 3; i++)
    {
        QPushButton* optBtn = new QPushButton(themeLabels[i], toggleGroup);
        optBtn->setFixedSize(60, 28);
        optBtn->setCursor(Qt::PointingHandCursor);
        optBtn->setFlat(true);
        optBtns[i] = optBtn;

        if (us->themeMode == i) {
            optBtn->setStyleSheet(
                "background: #3367D6; color: white; border: none; border-radius: 14px; font-size: 12px; font-weight: bold;"
            );
        } else {
            optBtn->setStyleSheet(
                QString("background: transparent; color: %1; border: none; border-radius: 14px; font-size: 12px;")
                    .arg(isDark ? "#aaa" : "#888")
            );
        }

        toggleLayout->addWidget(optBtn);
    }

    // 统一连接点击事件
    for (int i = 0; i < 3; i++)
    {
        connect(optBtns[i], &QPushButton::clicked, this, [this, i, optBtns, toggleGroup]{
            if (us->themeMode == i) return;
            us->themeMode = i;
            us->set("panel/themeMode", i);
            us->sync();

            bool dark = isDarkTheme();
            for (int j = 0; j < 3; j++)
            {
                if (j == i) {
                    optBtns[j]->setStyleSheet(
                        "background: #3367D6; color: white; border: none; border-radius: 14px; font-size: 12px; font-weight: bold;"
                    );
                } else {
                    optBtns[j]->setStyleSheet(
                        QString("background: transparent; color: %1; border: none; border-radius: 14px; font-size: 12px;")
                            .arg(dark ? "#aaa" : "#888")
                    );
                }
            }
            // 更新容器样式
            toggleGroup->setStyleSheet(dark
                ? "background: #2a2a2a; border: 1px solid #555; border-radius: 16px;"
                : "background: #f0f0f0; border: 1px solid #ddd; border-radius: 16px;"
            );

            // 触发主窗口主题更新
            emit themeChanged();
        });
    }

    hlayout->addWidget(toggleGroup);
    hlayout->addSpacing(12);

    btn->setFixedHeight(52);

    if (items.size())
    {
        QWidget* w = new QWidget(this);
        w->setFixedHeight(1);
        w->setStyleSheet(isDark ? "background-color: #444444;" : "background-color: #20888888");
        mainLayout->addWidget(w);
    }
    mainLayout->addWidget(btn);
    _lastItem = btn;
    items.append(btn);
    texts.append("主题颜色\n浅色/深色/跟随系统");
    return btn;
}

void SettingsItemListBox::paintEvent(QPaintEvent *)
{
    // 自定义控件的QSS无效，需要使用这个办法
    QStyleOption option;
    option.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &p, this);

}
