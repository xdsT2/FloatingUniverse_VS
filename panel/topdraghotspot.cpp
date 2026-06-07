#include "topdraghotspot.h"
#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>
#include <QCursor>
#include <QDebug>
#ifdef Q_OS_WIN32
#include <windows.h>
#endif

TopDragHotspot::TopDragHotspot(MainWindow *panel, int height, QWidget *parent)
    : QWidget(parent), m_height(height), m_panel(panel)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAcceptDrops(true);
    setMouseTracking(true);

    m_collapseTimer.setInterval(m_collapseDelayMs);
    m_collapseTimer.setSingleShot(true);
    connect(&m_collapseTimer, &QTimer::timeout, this, &TopDragHotspot::onCollapseTimeout);

    hide();
}

void TopDragHotspot::showHotspot()
{
    QScreen *screen = QApplication::primaryScreen();
    QRect geom = screen->geometry();
    setGeometry(geom.x(), geom.y(), geom.width(), m_height);

    // 强制置顶
    show();
    raise();
    activateWindow();

#ifdef Q_OS_WIN32
    // 确保热区在面板窗口 Z-order 之上
    if (m_panel) {
        HWND hHotspot = (HWND)winId();
        HWND hPanel = (HWND)m_panel->winId();
        if (hHotspot && hPanel) {
            SetWindowPos(hHotspot, hPanel, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
#endif

    qDebug() << "[TopDragHotspot] showHotspot at" << geometry();
}

void TopDragHotspot::hideHotspot()
{
    m_collapseTimer.stop();
    hide();
#ifdef Q_OS_WIN32
    if (m_panel) {
        HWND hPanel = (HWND)m_panel->winId();
        if (hPanel) {
            SetWindowPos(hPanel, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
#endif
    qDebug() << "[TopDragHotspot] hideHotspot";
}

void TopDragHotspot::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    m_collapseTimer.stop();
    if (m_panel) {
        // 确保热区置顶后再展开面板
        raise();
        m_panel->expandPanel();
        qDebug() << "[TopDragHotspot] enter -> expandPanel";
    }
}

void TopDragHotspot::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    // 延迟收起，允许鼠标移到 panel 上
    m_collapseTimer.start();
}

void TopDragHotspot::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    // 保持活跃，防止 leave 误判
}

void TopDragHotspot::onCollapseTimeout()
{
    if (!m_panel) return;
    // 动画进行中不收
    if (m_panel->isAnimating()) {
        m_collapseTimer.start(); // 重新启动
        return;
    }
    QPoint global = QCursor::pos();
    // 如果鼠标既不在 hotspot 也不在 panel 内，则收起
    bool inHotspot = geometry().contains(global);
    bool inPanel = false;
    if (m_panel->isVisible()) {
        inPanel = m_panel->geometry().contains(global);
    }
    if (!inHotspot && !inPanel) {
        m_panel->foldPanel();
        qDebug() << "[TopDragHotspot] collapse timeout -> foldPanel";
    } else {
        qDebug() << "[TopDragHotspot] collapse cancelled, mouse in panel/hotspot";
    }
}

void TopDragHotspot::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
        event->acceptProposedAction();
        emit requestShowPanel();
    } else {
        event->ignore();
    }
}

void TopDragHotspot::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void TopDragHotspot::dropEvent(QDropEvent *event)
{
    const QMimeData *m = event->mimeData();
    QList<QUrl> urls;
    if (m->hasUrls()) {
        urls = m->urls();
        event->acceptProposedAction();
        emit droppedUrls(urls);
    } else if (m->hasText()) {
        event->acceptProposedAction();
        emit droppedUrls(QList<QUrl>());
    } else {
        event->ignore();
    }
}
