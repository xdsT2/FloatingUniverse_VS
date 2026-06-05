#include "floatball.h"
#include "usettings.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QScreen>
#include <QGuiApplication>

FloatBall::FloatBall(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(60, 60);
    dragging = false;
    hovered = false;
}

void FloatBall::initBall()
{
    button = new WaterCircleButton(this);
    button->setFixedSize(50, 50);
    button->move(5, 5);
    button->setIcon(QIcon(":/icons/signin"));
    button->setBgColor(us->themeMainColor);
    button->setHoverColor(QColor(255, 255, 255, 50));
    button->setPressColor(QColor(255, 255, 255, 100));
    button->setFixedForePos();
    button->setFixedForeSize();
    button->setIconPaddingProper(0.25);
    button->setCursor(Qt::PointingHandCursor);
    button->installEventFilter(this);
    
    QRect screen = screenGeometry();
    move(screen.width() - 80, screen.height() - 150);
}

void FloatBall::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragging = false;
        dragPos = event->globalPosition().toPoint();
        startPos = this->pos();
        event->accept();
    }
}

void FloatBall::mouseMoveEvent(QMouseEvent *event)
{
    if (dragPos != QPoint()) {
        QPoint offset = event->globalPosition().toPoint() - dragPos;
        if (offset.manhattanLength() > QApplication::startDragDistance()) {
            dragging = true;
        }
    }
    if (dragging) {
        QPoint offset = event->globalPosition().toPoint() - dragPos;
        move(startPos + offset);
    }
    QWidget::mouseMoveEvent(event);
}

void FloatBall::mouseReleaseEvent(QMouseEvent *event)
{
    if (dragging) {
        dragging = false;
    }
    dragPos = QPoint();
    QWidget::mouseReleaseEvent(event);
}

bool FloatBall::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == button) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                dragging = false;
                dragPos = mouseEvent->globalPosition().toPoint();
                startPos = this->pos();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (dragPos != QPoint()) {
                QPoint offset = mouseEvent->globalPosition().toPoint() - dragPos;
                if (offset.manhattanLength() > QApplication::startDragDistance()) {
                    dragging = true;
                }
            }
            if (dragging) {
                QPoint offset = mouseEvent->globalPosition().toPoint() - dragPos;
                move(startPos + offset);
            }
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                bool wasDragging = dragging;
                dragging = false;
                dragPos = QPoint();
                if (!wasDragging) {
                    emit clicked();
                }
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void FloatBall::moveToEdge()
{
    QRect screen = screenGeometry();
    QRect geometry = this->geometry();
    
    int targetX = geometry.x();
    int centerX = geometry.center().x();
    
    if (centerX < screen.center().x()) {
        targetX = 0;
    } else {
        targetX = screen.width() - geometry.width();
    }
    
    QPropertyAnimation* ani = new QPropertyAnimation(this, "pos");
    ani->setStartValue(this->pos());
    ani->setEndValue(QPoint(targetX, geometry.y()));
    ani->setDuration(300);
    ani->setEasingCurve(QEasingCurve::OutCubic);
    ani->start(QAbstractAnimation::DeleteWhenStopped);
}

void FloatBall::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    if (hovered) {
        QPainterPath path;
        path.addEllipse(5, 5, 50, 50);
        painter.fillPath(path, QColor(255, 255, 255, 20));
    }
    
    QWidget::paintEvent(event);
}

void FloatBall::enterEvent(QEnterEvent *event)
{
    hovered = true;
    update();
    QWidget::enterEvent(event);
}

void FloatBall::leaveEvent(QEvent *event)
{
    hovered = false;
    update();
    QWidget::leaveEvent(event);
}

QRect FloatBall::screenGeometry() const
{
    auto screens = QGuiApplication::screens();
    int index = 0;
    if (index >= screens.size())
        index = screens.size() - 1;
    if (index < 0)
        return QRect();
    return screens.at(index)->geometry();
}
