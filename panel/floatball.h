#ifndef FLOATBALL_H
#define FLOATBALL_H

#include <QWidget>
#include <QPoint>
#include <QRect>
#include <QEnterEvent>
#include "watercirclebutton.h"

class FloatBall : public QWidget
{
    Q_OBJECT
public:
    explicit FloatBall(QWidget *parent = nullptr);
    void initBall();
    void moveToEdge();

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    WaterCircleButton* button;
    bool dragging;
    QPoint dragPos;
    QPoint startPos;
    bool hovered;
    QRect screenGeometry() const;
};

#endif // FLOATBALL_H
