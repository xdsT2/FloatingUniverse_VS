#include <QGraphicsDropShadowEffect>
#include <QScrollBar>
#include <QDir>
#include <QApplication>
#include <QSettings>
#include <QTimer>
#include "settingscontroller.h"
#include "ui_settingscontroller.h"
#include "usettings.h"
#include "signaltransfer.h"

#define UPDATE_PANEL connect(w->lastItem(), SIGNAL(clicked()), this, SIGNAL(updatePanel()))

SettingsController::SettingsController(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PanelSettingsWidget)
{
    ui->setupUi(this);

    initItems();
}

SettingsController::~SettingsController()
{
    delete ui;
}

void SettingsController::initItems()
{
    // 一、视觉与外观
    auto w = new SettingsItemListBox(ui->scrollAreaWidgetContents);
    // 主题选择 - 三态切换
    w->addThreeStateTheme();
    UPDATE_PANEL;
    connect(w, &SettingsItemListBox::themeChanged, this, [=]{
        emit updatePanel();
        emit themeModeChanged(us->themeMode);
    });
    addGroup(w, "视觉与外观");

    // 二、核心操作
    w = new SettingsItemListBox(ui->scrollAreaWidgetContents);
    w->add(QPixmap(":/icons/st/moveOut"), "允许拖拽到外面", "开启后，支持将面板内的图标直接拖拽释放到桌面、微信、PS等外部程序", "interactive/allowMoveOut", &us->allowMoveOut);
    w->add(QPixmap(":/icons/st/fileName"), "重命名同步源文件", "在手推车内右键重命名文件时，是否自动联动修改电脑硬盘里的源文件名称", "file/modifyFileNameSync", &us->modifyFileNameSync);
    addGroup(w, "核心操作");

    // 三、系统与杂项
    w = new SettingsItemListBox(ui->scrollAreaWidgetContents);
    w->add(QPixmap(":/icons/st/reboot"), "开机自启", "随Windows系统启动并在托盘静默运行", "interactive/autoReboot", &us->autoReboot);
    connect(w->lastItem(), &InteractiveButtonBase::clicked, this, [=]{
        QString appName = QApplication::applicationName();
        QString appPath = QDir::toNativeSeparators(QApplication::applicationFilePath());
        QSettings *reg = new QSettings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        QString val = reg->value(appName).toString();
        if (us->autoReboot)
            reg->setValue(appName, appPath);
        else
            reg->remove(appName);
        qInfo() << "设置自启：" << us->autoReboot;
        reg->sync();
        reg->deleteLater();
    });
    addGroup(w, "系统与杂项");

    // 关于程序
    w = new SettingsItemListBox(ui->scrollAreaWidgetContents);
    w->addOpen(QPixmap(":/icons/lyixi"), "开发者", "杭州懒一夕智能科技有限公司", QUrl("http://lyixi.com"));
    w->addOpen(QPixmap(":/icons/st/QQ"), "交流反馈", "QQ群：280517990", QUrl("https://qm.qq.com/cgi-bin/qm/qr?k=a3rJlTLgGAhgx5PqvHz0RjinfHDpl4Ll&jump_from=webapi"));
    addGroup(w, "关于程序");
}

void SettingsController::addGroup(QWidget *w, QString name)
{
    bool isDark = isDarkTheme();

    QLabel* label = new QLabel(name, ui->scrollAreaWidgetContents);
    label->setStyleSheet(isDark ? "color: #e0e0e0; margin: 0.3px;" : "color: #202020; margin: 0.3px;");
    w->setObjectName("SettingsGroup");
    if (isDark)
    {
        w->setStyleSheet("#SettingsGroup, SettingsItemListBox{ background: #2d2d2d; border: none; border-radius: 5px; }");
    }
    else
    {
        w->setStyleSheet("#SettingsGroup, SettingsItemListBox{ background: white; border: none; border-radius: 5px; }");
    }
    w->adjustSize();

    labels.append(label);
    boxes.append(w);
}

void SettingsController::focusGroup(int index)
{
    if (index < 0 || index >= labels.size())
        return ;

    int margin = labels.first()->y();
    int top = labels.at(index)->y() - margin;
    ui->scrollArea->scrollTo(top);
}

void SettingsController::adjustGroupSize()
{
    int margin = us->settingsMargin;
    const int fixedWidth = qMin(qMax(ui->scrollAreaWidgetContents->width() - margin * 2, us->settingsMinWidth), us->settingsMaxWidth);
    const int groupSpacing = 18;
    const int labelSpacing = 9;
    const int left = qMax(margin, (ui->scrollAreaWidgetContents->width() - fixedWidth) / 2);
    int top = groupSpacing;
    for (int i = 0; i < labels.size(); i++)
    {
        auto label = labels.at(i);
        auto box = boxes.at(i);
        label->setFixedWidth(fixedWidth);
        box->setFixedWidth(fixedWidth);
        label->adjustSize();
        box->adjustSize();
        box->setMinimumHeight(64);

        label->move(left, top);
        top += label->height() + labelSpacing;

        box->move(left, top);
        top += box->height() + groupSpacing;
    }

    ui->scrollAreaWidgetContents->setFixedHeight(top + labelSpacing);

    emit boxH(left, fixedWidth);
}

void SettingsController::setFind(QString key)
{
    bool finded = false;
    for (int i = 0; i < boxes.size(); i++)
    {
        auto w = qobject_cast<SettingsItemListBox*>(boxes.at(i));
        if (!w)
            continue;
        int rst = w->setFind(key);
        if (!finded && rst > -1)
        {
            focusGroup(i);
            finded = true;
        }
    }
}

void SettingsController::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    adjustGroupSize();
}

void SettingsController::updateTheme()
{
    bool isDark = isDarkTheme();

    // 更新组标签颜色
    for (auto label : labels)
        label->setStyleSheet(isDark ? "color: #e0e0e0; margin: 0.3px;" : "color: #202020; margin: 0.3px;");

    // 更新组背景色和阴影
    for (auto box : boxes)
    {
        if (isDark)
        {
            box->setStyleSheet("#SettingsGroup, SettingsItemListBox{ background: #2d2d2d; border: none; border-radius: 5px; }");
        }
        else
        {
            box->setStyleSheet("#SettingsGroup, SettingsItemListBox{ background: white; border: none; border-radius: 5px; }");
        }
        // 更新阴影颜色
        auto effect = box->graphicsEffect();
        if (effect)
        {
            QGraphicsDropShadowEffect* shadow = qobject_cast<QGraphicsDropShadowEffect*>(effect);
            if (shadow)
                shadow->setColor(isDark ? Qt::darkGray : Qt::lightGray);
        }
    }

    // 更新每个列表项内部文字颜色
    for (auto box : boxes)
    {
        SettingsItemListBox* listBox = qobject_cast<SettingsItemListBox*>(box);
        if (listBox)
            listBox->updateTheme();
    }

    update();
}
