#include "topdraghotspot.h"
#include <QApplication>
#include <QScreen>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>

TopDragHotspot::TopDragHotspot(int height, QWidget *parent)
    : QWidget(parent), m_height(height)
{
    // 窗口样式：无边框、工具窗口、总在最上层；不出现在任务栏
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAcceptDrops(true);

    // 放在屏幕顶端（使用主屏幕）
    QScreen *screen = QApplication::primaryScreen();
    QRect geom = screen->geometry();
    setGeometry(geom.x(), geom.y(), geom.width(), m_height);
    show(); // 一直显示一个很窄的热区
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
        // 文本类型也接受
        event->acceptProposedAction();
        // 文本可以通过 droppedUrls 或者新增信号传递
        emit droppedUrls(QList<QUrl>());
    } else {
        event->ignore();
    }
}

void TopDragHotspot::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    // 拖拽离开热区时不处理，由主面板接管
}
