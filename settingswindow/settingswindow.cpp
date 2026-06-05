#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include "facilemenu.h"
#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "usettings.h"
#include "myjson.h"
#include "fileutil.h"
#include "imageutil.h"
#include "netutil.h"
#include "signaltransfer.h"
#include <QSettings>
#ifdef Q_OS_WIN32
#include "windows.h"
#include <dwmapi.h>
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif
#include "runtime.h"

SettingsWindow::SettingsWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);

    // 使用无边框窗口，自定义标题栏
    setWindowFlags(Qt::FramelessWindowHint);

    // 确保窗口有原生句柄
    (void)winId();

    initView();
    initTray();
    initPanel();

    // 延迟应用主题，确保所有子控件已创建
    QTimer::singleShot(0, this, [=]{
        applyWindowTheme();
    });

    us->set("usage/bootCount", ++us->bootCount);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
    delete panel;
    delete floatBall;
    delete tray;
    deleteDir(rt->CACHE_PATH);
}

void SettingsWindow::initView()
{
    // 连接标题栏按钮
    connect(ui->btnMinimize, &QPushButton::clicked, this, [=]{
        this->showMinimized();
    });
    connect(ui->btnClose, &QPushButton::clicked, this, [=]{
        this->hide();
    });
}

void SettingsWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && ui->titleBar->rect().contains(ui->titleBar->mapFromGlobal(event->globalPosition().toPoint())))
    {
        m_dragging = true;
        m_dragPosition = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
        event->accept();
    }
    QMainWindow::mousePressEvent(event);
}

void SettingsWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && event->buttons() & Qt::LeftButton)
    {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
    QMainWindow::mouseMoveEvent(event);
}

void SettingsWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    QMainWindow::mouseReleaseEvent(event);
}

void SettingsWindow::applyWindowTheme()
{
    bool isDark = isDarkTheme();

    if (isDark)
    {
        // === 暗色主题 ===
        // 设置窗口和中央区域
        this->setStyleSheet("#SettingsWindow { background: #1e1e1e; }");
        if (ui->centralwidget)
            ui->centralwidget->setStyleSheet("#centralwidget { background: #2d2d2d; }");

        // 标题栏
        if (ui->titleBar)
            ui->titleBar->setStyleSheet("#titleBar { background: #2d2d2d; }");
        if (ui->titleLabel)
            ui->titleLabel->setStyleSheet("color: #e0e0e0; font-size: 13px;");
        if (ui->btnMinimize)
            ui->btnMinimize->setStyleSheet(
                "QPushButton { background: transparent; border: none; color: #e0e0e0; font-size: 16px; }"
                "QPushButton:hover { background: #404040; }"
            );
        if (ui->btnClose)
            ui->btnClose->setStyleSheet(
                "QPushButton { background: transparent; border: none; color: #e0e0e0; font-size: 12px; }"
                "QPushButton:hover { background: #e81123; color: white; }"
            );
    }
    else
    {
        // === 亮色主题 ===
        this->setStyleSheet("#SettingsWindow { background: #f8f9fa; }");
        if (ui->centralwidget)
            ui->centralwidget->setStyleSheet("#centralwidget { background: white; }");

        // 标题栏
        if (ui->titleBar)
            ui->titleBar->setStyleSheet("#titleBar { background: #f0f0f0; }");
        if (ui->titleLabel)
            ui->titleLabel->setStyleSheet("color: #202020; font-size: 13px;");
        if (ui->btnMinimize)
            ui->btnMinimize->setStyleSheet(
                "QPushButton { background: transparent; border: none; color: #202020; font-size: 16px; }"
                "QPushButton:hover { background: #e0e0e0; }"
            );
        if (ui->btnClose)
            ui->btnClose->setStyleSheet(
                "QPushButton { background: transparent; border: none; color: #202020; font-size: 12px; }"
                "QPushButton:hover { background: #e81123; color: white; }"
            );
    }
}

QRect SettingsWindow::screenGeometry() const
{
    auto screens = QGuiApplication::screens();
    int index = 0;
    if (index >= screens.size())
        index = screens.size() - 1;
    if (index < 0)
        return QRect();
    return screens.at(index)->geometry();
}

void SettingsWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
    us->set("mainwindow/hide", false);

    QTimer::singleShot(0, [=]{
        ui->settingsBody->adjustGroupSize();
    });

    static bool first = true;
    if (first)
    {
        first = false;
        restoreGeometry(us->value("mainwindow/geometry").toByteArray());
        restoreState(us->value("mainwindow/state").toByteArray());
    }

    applyWindowTheme();
}

void SettingsWindow::initTray()
{
    tray = new QSystemTrayIcon(this);
    tray->setIcon(QIcon(":/icons/appicon"));
    tray->setToolTip(APPLICATION_NAME);
    tray->show();

    connect(tray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(trayAction(QSystemTrayIcon::ActivationReason)));
}

void SettingsWindow::trayAction(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    case QSystemTrayIcon::Trigger:
        if (!us->trayClickOpenPanel)
        {
            if (!this->isHidden())
                this->hide();
            else
            {
                this->showNormal();
                this->activateWindow();
            }
        }
        else
        {
            togglePanelVisibility();
        }
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    case QSystemTrayIcon::Context:
    {
        FacileMenu* menu = new FacileMenu;

        menu->addAction(QIcon(":/icons/show_panel"), "唤出", [=]{
            togglePanelVisibility();
        });

        menu->addAction(QIcon(":/icons/hide"), "隐藏", [=]{
            togglePanelVisibility();
        });

        menu->split()->addAction(QIcon(":/icons/quit"), "退出", [=]{
            qApp->quit();
        });

        menu->exec(QCursor::pos());
    }
        break;
    default:
        break;
    }
}

void SettingsWindow::initKey()
{
#if defined(ENABLE_SHORTCUT)
    editShortcut = new QxtGlobalShortcut(this);
    QString def_key = us->value("banner/replyKey", "shift+alt+x").toString();
    editShortcut->setShortcut(QKeySequence(def_key));
    connect(editShortcut, &QxtGlobalShortcut::activated, this, [=]() {
#if defined(Q_OS_WIN32)

#endif
        // this->activateWindow();
    });
#endif
}

void SettingsWindow::initPanel()
{
    // 创建面板
    panel = new MainWindow(nullptr);
    connect(panel, SIGNAL(openSettings()), this, SLOT(show()));
    panel->initPanel();
    panel->show();
    connect(ui->settingsBody, SIGNAL(updatePanel()), panel, SLOT(update()));
    connect(ui->settingsBody, &SettingsController::themeModeChanged, this, [=](){
        applyWindowTheme();
        ui->settingsBody->updateTheme();
        panel->detectAndApplySystemTheme();
    });
    
    // 创建悬浮球
    floatBall = new FloatBall(nullptr);
    floatBall->initBall();
    floatBall->hide(); // 默认隐藏
    
    // 连接悬浮球点击信号 - 单击展开面板
    connect(floatBall, &FloatBall::clicked, this, &SettingsWindow::togglePanelVisibility);
    
    // 连接面板双击信号 - 收缩到悬浮球
    connect(panel, &MainWindow::minimizeToBall, this, [=](){
        panel->hide();
        // 在面板当前位置显示悬浮球
        floatBall->move(panel->pos());
        floatBall->show();
    });
    
    connect(panel, &MainWindow::panelPositionChanged, this, [=](){
        // 面板移动时更新悬浮球位置（如果悬浮球显示的话）
        if (floatBall->isVisible()) {
            floatBall->move(panel->pos());
        }
    });
}

void SettingsWindow::togglePanelVisibility()
{
    if (panel && floatBall) {
        if (panel->isHidden()) {
            // 显示面板，隐藏悬浮球
            panel->showPanel();
            floatBall->hide();
        } else {
            // 隐藏面板，显示悬浮球
            panel->hide();
            floatBall->move(panel->pos());
            floatBall->show();
        }
    }
}

void SettingsWindow::closeEvent(QCloseEvent *e)
{
    us->setValue("mainwindow/geometry", this->saveGeometry());
    us->setValue("mainwindow/state", this->saveState());
	us->sync();

#if defined(ENABLE_TRAY)
    e->ignore();
    this->hide();

    // 因为关闭程序也会触发这个，所以需要定时一下
    QTimer::singleShot(5000, [=]{
        us->set("mainwindow/hide", true);
    });

    QTimer::singleShot(5000, [=]{
        if (!this->isHidden())
            return ;
        us->setValue("mainwindow/autoShow", false);
    });
#endif
    QMainWindow::closeEvent(e);
}

void SettingsWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
}

void SettingsWindow::returnToPrevWindow()
{
#ifdef Q_OS_WIN32
        if (this->prevWindow)
            SwitchToThisWindow(prevWindow, true);
        prevWindow = nullptr;
#endif
}

#ifdef Q_OS_WIN32
bool SettingsWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    MSG *msg = static_cast<MSG*>(message);
    if (msg && msg->message == WM_SETTINGCHANGE)
    {
        // 检测系统主题变化
        if (msg->lParam != 0)
        {
            QString section = QString::fromWCharArray(reinterpret_cast<wchar_t*>(msg->lParam));
            if (section == "ImmersiveColorSet")
            {
                QTimer::singleShot(200, this, [=]{
                    applyWindowTheme();
                    if (ui->settingsBody)
                        ui->settingsBody->updateTheme();
                    // 同步更新主面板主题
                    if (panel)
                        panel->detectAndApplySystemTheme();
                });
            }
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif
