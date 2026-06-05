#include "mainwindow.h"
#include "runtime.h"
#include "usettings.h"
#include "signaltransfer.h"
#include "universepanel.h"
#include "icontextitem.h"
#include <QScreen>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QLabel>
#include <QApplication>
#include <QPalette>
#include <QSettings>
#ifdef Q_OS_WIN32
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#endif

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(true);
    dragging = false;
    titleDragging = false;
}

void MainWindow::initPanel()
{
    QRect screen = screenGeometry();
    resize(us->panelWidth, us->panelHeight);
    move((screen.width() - width()) / 2 + us->panelCenterOffset, 50);
    expanding = true;
    fixing = true;
    _prev_fixing = true;

    rt->panel_expading = &expanding;
    rt->panel_animating = &animating;
    rt->panel_fixing = &fixing;

    // 检测并应用系统主题
    detectAndApplySystemTheme();

    // 创建顶栏
    initTitleBar();

    panel = new UniversePanel(this);
    panel->initPanel();
    connect(panel, SIGNAL(openSettings()), this, SIGNAL(openSettings()));
    connect(panel, &UniversePanel::signalFoldPanel, this, [=] { foldPanel(); });
    connect(panel, &UniversePanel::signalExpandPanel, this, [=] { expandPanel(); });
    connect(panel, &UniversePanel::signalToggleFixed, this, [=] {
        fixing = !fixing;
    });
    connect(panel, &UniversePanel::signalSetKeepFix, this, [=](bool keep) {
        if (keep)
        {
            _prev_fixing = fixing;
            fixing = true;
        }
        else
        {
            auto f = _prev_fixing;
            QTimer::singleShot(0, [=]{
                fixing = f;
            });
        }
    });
    connect(panel, &UniversePanel::signalKeepPanelState, this, [=](FuncType func) {
        keepPanelState(func);
    });
}

void MainWindow::initTitleBar()
{
    titleBar = new QWidget(this);
    titleBar->setObjectName("TitleBar");
    titleBar->setFixedHeight(32);
    titleBar->setCursor(Qt::SizeAllCursor);
    titleBar->setMouseTracking(true);
    titleBar->installEventFilter(this);
    
    // 设置顶栏样式，适配系统主题
    bool isDark = isDarkTheme();
    if (isDark) {
        titleBar->setStyleSheet(
            "#TitleBar { background-color: rgba(32, 32, 32, 0.95); border-bottom: 1px solid rgba(255,255,255,0.1); }"
        );
    } else {
        titleBar->setStyleSheet(
            "#TitleBar { background-color: rgba(243, 243, 243, 0.95); border-bottom: 1px solid rgba(0,0,0,0.1); }"
        );
    }

    // 小巴掌图标 - 拖拽提示
    dragHintBtn = new QPushButton(titleBar);
    dragHintBtn->setObjectName("DragHintBtn");
    dragHintBtn->setFixedSize(28, 28);
    dragHintBtn->setIcon(QIcon(":/icons/happy"));
    dragHintBtn->setIconSize(QSize(20, 20));
    dragHintBtn->setCursor(Qt::PointingHandCursor);
    dragHintBtn->setToolTip("可以拖拽");
    dragHintBtn->setFlat(true);
    
    if (isDark) {
        dragHintBtn->setStyleSheet(
            "#DragHintBtn { background: transparent; border: none; }"
            "#DragHintBtn:hover { background: rgba(255,255,255,0.1); border-radius: 4px; }"
        );
    } else {
        dragHintBtn->setStyleSheet(
            "#DragHintBtn { background: transparent; border: none; }"
            "#DragHintBtn:hover { background: rgba(0,0,0,0.05); border-radius: 4px; }"
        );
    }

    // 小眼睛图标 - 悬浮窗切换
    floatBtn = new QPushButton(titleBar);
    floatBtn->setObjectName("FloatBtn");
    floatBtn->setFixedSize(28, 28);
    floatBtn->setIcon(QIcon(":/icons/eye"));
    floatBtn->setIconSize(QSize(20, 20));
    floatBtn->setCursor(Qt::PointingHandCursor);
    floatBtn->setToolTip("切换悬浮窗");
    floatBtn->setFlat(true);
    
    if (isDark) {
        floatBtn->setStyleSheet(
            "#FloatBtn { background: transparent; border: none; }"
            "#FloatBtn:hover { background: rgba(255,255,255,0.1); border-radius: 4px; }"
        );
    } else {
        floatBtn->setStyleSheet(
            "#FloatBtn { background: transparent; border: none; }"
            "#FloatBtn:hover { background: rgba(0,0,0,0.05); border-radius: 4px; }"
        );
    }
    
    connect(floatBtn, &QPushButton::clicked, this, [this] {
        emit minimizeToBall();
    });

    // 设置图标 - 打开设置
    settingsBtn = new QPushButton(titleBar);
    settingsBtn->setObjectName("SettingsBtn");
    settingsBtn->setFixedSize(28, 28);
    
    // 根据面板背景色创建对应颜色的图标
    QIcon originalIcon(":/icons/settings");
    QPixmap pixmap = originalIcon.pixmap(20, 20);
    if (!pixmap.isNull()) {
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        // 白色面板用深色图标，黑色面板用浅色图标
        QColor iconColor = isDark ? QColor(255, 255, 255) : QColor(50, 50, 50);
        painter.fillRect(pixmap.rect(), iconColor);
        settingsBtn->setIcon(QIcon(pixmap));
    }
    settingsBtn->setIconSize(QSize(20, 20));
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setToolTip("设置");
    settingsBtn->setFlat(true);
    
    if (isDark) {
        settingsBtn->setStyleSheet(
            "#SettingsBtn { background: rgba(255,255,255,0.1); border: none; border-radius: 4px; }"
            "#SettingsBtn:hover { background: rgba(255,255,255,0.2); border-radius: 4px; }"
        );
    } else {
        settingsBtn->setStyleSheet(
            "#SettingsBtn { background: rgba(0,0,0,0.05); border: none; border-radius: 4px; }"
            "#SettingsBtn:hover { background: rgba(0,0,0,0.1); border-radius: 4px; }"
        );
    }
    
    connect(settingsBtn, &QPushButton::clicked, this, [this] {
        emit openSettings();
    });
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // 中键拖动面板
    if (event->button() == Qt::MiddleButton) {
        dragging = true;
        dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == titleBar) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                titleDragging = true;
                titleDragged = false;
                titleDragPos = mouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
                event->accept();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            if (titleDragging) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                move(mouseEvent->globalPosition().toPoint() - titleDragPos);
                emit panelPositionChanged();
                if (QPoint(mouseEvent->globalPosition().toPoint() - (titleDragPos + frameGeometry().topLeft())).manhattanLength() > 5) {
                    titleDragged = true;
                }
                event->accept();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                titleDragging = false;
                if (titleDragged) {
                    event->accept();
                    titleDragged = false;
                    return true;
                }
                titleDragged = false;
                event->accept();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                emit minimizeToBall();
                event->accept();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 中键拖动
    if (dragging && (event->buttons() & Qt::MiddleButton)) {
        move(event->globalPosition().toPoint() - dragPos);
        emit panelPositionChanged();
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        dragging = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit minimizeToBall();
        event->accept();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void MainWindow::showPanel()
{
    QRect screen = screenGeometry();
    resize(us->panelWidth, us->panelHeight);
    move((screen.width() - width()) / 2 + us->panelCenterOffset, 50);
    expanding = true;
    fixing = true;
    _prev_fixing = true;
    this->show();
    this->raise();
}

void MainWindow::expandPanel()
{
    if (expanding)
        return;

    if (us->panelBlur && this->pos().y() <= -this->height() + 1)
    {
        int radius = us->panelBlurRadius;
        QScreen* screen = QApplication::screenAt(QCursor::pos());
        QPixmap bg = screen->grabWindow(0,
                                        pos().x() - radius,
                                        0 - radius,
                                        width() + radius * 2,
                                        height() + radius * 2);

        QT_BEGIN_NAMESPACE
            extern Q_WIDGETS_EXPORT void qt_blurImage( QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
        QT_END_NAMESPACE

        QPixmap pixmap = bg;
        QPainter painter( &pixmap );
        QImage img = pixmap.toImage();
        qt_blurImage( &painter, img, radius, true, false );

        QPixmap blured(pixmap.size());
        blured.fill(Qt::transparent);
        QPainter painter2(&blured);
        painter2.setOpacity(us->panelBlurOpacity / 255.0);
        painter2.drawPixmap(blured.rect(), pixmap);

        int c = qMin(bg.width(), bg.height());
        c = qMin(c/2, radius);
        panelBlurPixmap = blured.copy(c, c, blured.width()-c*2, blured.height()-c*2);
    }

    QPropertyAnimation* ani = new QPropertyAnimation(this, "pos");
    ani->setStartValue(pos());
    ani->setEndValue(QPoint(pos().x(), 0));
    ani->setDuration(300);
    ani->setEasingCurve(QEasingCurve::OutCubic);
    if (us->panelBlur)
    {
        connect(ani, &QPropertyAnimation::valueChanged, this, [=]{
            update();
        });
    }
    connect(ani, &QPropertyAnimation::finished, this, [=]{
        ani->deleteLater();
        PanelItemBase::_blockPress = animating = false;
        update();
    });
    ani->start();
    expanding = true;
    PanelItemBase::_blockPress = animating = true;
}

void MainWindow::foldPanel()
{
    if (fixing)
        return ;

    QPropertyAnimation* ani = new QPropertyAnimation(this, "pos");
    ani->setStartValue(pos());
    ani->setEndValue(QPoint(pos().x(), -height() + us->panelBangHeight));
    ani->setDuration(300);
    ani->setEasingCurve(QEasingCurve::InOutCubic);
    if (us->panelBlur)
    {
        connect(ani, &QPropertyAnimation::valueChanged, this, [=]{
            update();
        });
    }
    connect(ani, &QPropertyAnimation::finished, this, [=]{
        ani->deleteLater();
        PanelItemBase::_blockPress = animating = false;
        update();
    });
    ani->start();
    expanding = false;
    PanelItemBase::_blockPress = animating = true;
}

void MainWindow::keepPanelState(FuncType func)
{
    bool _fixing = fixing;
    fixing = true;
    func();
    fixing = _fixing;
}

void MainWindow::setPanelFixed(bool f)
{
    fixing = f;
}

void MainWindow::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);

    if (expanding)
        return ;

    int x = QCursor::pos().x() - this->x();
    if (x >= us->panelBangMarginLeft && x <= width() - us->panelBangMarginRight)
        expandPanel();
}

void MainWindow::leaveEvent(QEvent *event)
{
    if (us->allowMoveOut && panel && (panel->pressing || panel->scening || panel->isDragging))
    {
        return ;
    }

    if (us->allowMoveOut && panel->_release_outter)
    {
        panel->_release_outter = false;
        return ;
    }

    if (us->keepOnItemUsing && panel->hasItemUsing())
        return ;

    if (panel->currentMenu && panel->currentMenu->hasFocus())
        return ;

    QWidget::leaveEvent(event);

    if (expanding)
        foldPanel();
}

void MainWindow::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 检测主题颜色
    bool isDark = isDarkTheme();

    QColor bgColor = isDark ? QColor(32, 32, 32) : QColor(252, 252, 252);
    QColor bangBg = QColor(235, 235, 235);  // 刘海始终使用浅色，便于在深色壁纸上找到

    // 画主面板
    {
        QPainterPath path;
        path.addRoundedRect(QRect(0, 0, width(), height() - us->panelBangHeight), us->fluentRadius, us->fluentRadius);
        painter.fillPath(path, bgColor);

        if (us->panelBlur && us->panelBlurOpacity && !panelBlurPixmap.isNull())
        {
            QRect rect = this->rect();
            rect.moveTop(-this->y());
            painter.drawPixmap(rect, panelBlurPixmap);
        }
    }

    // 画安全框（拖拽时或点击触发后显示）
    if (us->allowMoveOut && panel && (safetyFrameVisible || panel->pressing || panel->scening || panel->isDragging))
    {
        QRect safetyRect = getSafetyFrameRect();
        if (safetyRect.isValid() && safetyRect.width() > 0 && safetyRect.height() > 0)
        {
            // 根据面板背景色选择线条颜色
            QColor lineColor;
            if (canDragOut) {
                // 绿色：允许拖出，但降低饱和度使其更柔和
                lineColor = QColor(76, 175, 80, 140);
            } else {
                // 白色/黑色：在安全框内，降低透明度
                lineColor = isDark ? QColor(255, 255, 255, 100) : QColor(0, 0, 0, 100);
            }
            
            QPen pen(lineColor, 1, Qt::DashLine);
            painter.setPen(pen);
            
            // 绘制圆角安全框
            QPainterPath safetyPath;
            safetyPath.addRoundedRect(safetyRect, us->fluentRadius - 2, us->fluentRadius - 2);
            painter.drawPath(safetyPath);
        }
    }

    // 画刘海
    if (this->pos().y() <= -this->height() + us->panelBangHeight)
    {
        QPainterPath path;
        path.addRect(us->panelBangMarginLeft, height() - us->panelBangHeight, qAbs(width() - us->panelBangMarginLeft - us->panelBangMarginRight), us->panelBangHeight);
        painter.fillPath(path, bangBg);
    }

    QWidget::paintEvent(e);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (titleBar) {
        titleBar->setGeometry(0, 0, width(), 32);
        if (dragHintBtn) {
            dragHintBtn->move(4, 2);
        }
        if (settingsBtn) {
            settingsBtn->move(width() - 64, 2);
        }
        if (floatBtn) {
            floatBtn->move(width() - 32, 2);
        }
    }

    if (panel)
    {
        int topOffset = titleBar ? 32 : 0;
        panel->setGeometry(0, topOffset, this->width(), this->height() - us->panelBangHeight - topOffset);
    }
}

void MainWindow::closeEvent(QCloseEvent *)
{
    return ;
}

void MainWindow::detectAndApplySystemTheme()
{
    bool isDark = isDarkTheme();

    // 1. 更新全局调色板
    if (isDark) {
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(32, 32, 32));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(45, 45, 45));
        darkPalette.setColor(QPalette::AlternateBase, QColor(32, 32, 32));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(45, 45, 45));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(darkPalette);
    } else {
        // 浅色主题 - 恢复默认调色板
        qApp->setPalette(QApplication::style()->standardPalette());
    }

    // 2. 更新标题栏样式
    if (titleBar) {
        if (isDark) {
            titleBar->setStyleSheet(
                "#TitleBar { background-color: rgba(32, 32, 32, 0.95); border-bottom: 1px solid rgba(255,255,255,0.1); }"
            );
        } else {
            titleBar->setStyleSheet(
                "#TitleBar { background-color: rgba(243, 243, 243, 0.95); border-bottom: 1px solid rgba(0,0,0,0.1); }"
            );
        }
    }

    // 3. 更新按钮样式
    if (dragHintBtn) {
        if (isDark) {
            dragHintBtn->setStyleSheet(
                "#DragHintBtn { background: transparent; border: none; }"
                "#DragHintBtn:hover { background: rgba(255,255,255,0.1); border-radius: 4px; }"
            );
        } else {
            dragHintBtn->setStyleSheet(
                "#DragHintBtn { background: transparent; border: none; }"
                "#DragHintBtn:hover { background: rgba(0,0,0,0.05); border-radius: 4px; }"
            );
        }
    }

    if (floatBtn) {
        if (isDark) {
            floatBtn->setStyleSheet(
                "#FloatBtn { background: transparent; border: none; }"
                "#FloatBtn:hover { background: rgba(255,255,255,0.1); border-radius: 4px; }"
            );
        } else {
            floatBtn->setStyleSheet(
                "#FloatBtn { background: transparent; border: none; }"
                "#FloatBtn:hover { background: rgba(0,0,0,0.05); border-radius: 4px; }"
            );
        }
    }

    if (settingsBtn) {
        if (isDark) {
            settingsBtn->setStyleSheet(
                "#SettingsBtn { background: rgba(255,255,255,0.1); border: none; border-radius: 4px; }"
                "#SettingsBtn:hover { background: rgba(255,255,255,0.2); border-radius: 4px; }"
            );
        } else {
            settingsBtn->setStyleSheet(
                "#SettingsBtn { background: rgba(0,0,0,0.05); border: none; border-radius: 4px; }"
                "#SettingsBtn:hover { background: rgba(0,0,0,0.1); border-radius: 4px; }"
            );
        }
        // 更新图标颜色
        QIcon originalIcon(":/icons/settings");
        QPixmap pixmap = originalIcon.pixmap(20, 20);
        if (!pixmap.isNull()) {
            QPainter painter(&pixmap);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            QColor iconColor = isDark ? QColor(255, 255, 255) : QColor(50, 50, 50);
            painter.fillRect(pixmap.rect(), iconColor);
            settingsBtn->setIcon(QIcon(pixmap));
        }
    }

    // 4. 更新所有文件项的文字颜色
    if (panel) {
        for (auto item : panel->items) {
            IconTextItem* iconTextItem = qobject_cast<IconTextItem*>(item);
            if (iconTextItem) {
                iconTextItem->updateTextColor();
            }
        }
    }

    // 5. 强制重绘面板
    update();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#else
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
#endif
{
#ifdef Q_OS_WIN32
    Q_UNUSED(eventType)
    MSG* msg = static_cast<MSG*>(message);
    switch(msg->message)
    {
    case WM_NCHITTEST:
        if (!fixing)
            return false;
        const auto ratio = devicePixelRatioF();
        int xPos = static_cast<int>(GET_X_LPARAM(msg->lParam) / ratio - this->frameGeometry().x());
        int yPos = static_cast<int>(GET_Y_LPARAM(msg->lParam) / ratio - this->frameGeometry().y());
        if(xPos < boundaryWidth && yPos < boundaryWidth)
            *result = HTTOPLEFT;
        else if(xPos >= width() - boundaryWidth && yPos < boundaryWidth)
            *result = HTTOPRIGHT;
        else if(xPos < boundaryWidth && yPos >= height() - boundaryWidth)
            *result = HTBOTTOMLEFT;
        else if(xPos >= width() - boundaryWidth && yPos >= height() - boundaryWidth)
            *result = HTBOTTOMRIGHT;
        else if(xPos < boundaryWidth)
            *result =  HTLEFT;
        else if(xPos >= width() - boundaryWidth)
            *result = HTRIGHT;
        else if(yPos >= height() - boundaryWidth)
        {
            *result = HTBOTTOM;
        }
        else
           return false;

        QRect screen = screenGeometry();
        us->set("panel/centerOffset", geometry().center().x() - screen.center().x());
        us->set("panel/width", width());
        us->set("panel/height", height());

        return true;
    }
#else
    return QWidget::nativeEvent(eventType, message, result);
#endif
    return false;
}

QRect MainWindow::screenGeometry() const
{
    auto screens = QGuiApplication::screens();
    int index = 0;
    if (index >= screens.size())
        index = screens.size() - 1;
    if (index < 0)
        return QRect();
    return screens.at(index)->geometry();
}

// 安全框相关实现
QRect MainWindow::getSafetyFrameRect() const
{
    if (!titleBar)
        return QRect();
    
    // 安全框是面板内部扣除边距后的矩形区域
    return QRect(safetyMargin,
                 safetyMargin + titleBar->height(),
                 width() - safetyMargin * 2,
                 height() - titleBar->height() - safetyMargin * 2 - us->panelBangHeight);
}

bool MainWindow::isItemOutsideSafetyFrame(const QRect& itemRect) const
{
    QRect safetyRect = getSafetyFrameRect();
    // 判断元素是否完全在安全框外（即不与安全框有任何重叠）
    return !itemRect.intersects(safetyRect);
}
