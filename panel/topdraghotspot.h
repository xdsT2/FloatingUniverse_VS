#ifndef TOPDRAGHOTSPOT_H
#define TOPDRAGHOTSPOT_H

#include <QWidget>
#include <QList>
#include <QUrl>
#include <QTimer>

class MainWindow;

class TopDragHotspot : public QWidget {
    Q_OBJECT
public:
    explicit TopDragHotspot(MainWindow *panel, int height = 6, QWidget *parent = nullptr);

    void showHotspot();
    void hideHotspot();
    void stopCollapseTimer() { m_collapseTimer.stop(); }

signals:
    void requestShowPanel();               // 拖拽到热区，要求下拉主面板
    void droppedUrls(const QList<QUrl>&);  // 用户在热区放开文件，转发 urls

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onCollapseTimeout();

private:
    int m_height;
    MainWindow *m_panel;
    QTimer m_collapseTimer;
};

#endif // TOPDRAGHOTSPOT_H
