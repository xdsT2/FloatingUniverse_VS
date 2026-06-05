#ifndef TOPDRAGHOTSPOT_H
#define TOPDRAGHOTSPOT_H

#include <QWidget>
#include <QList>
#include <QUrl>

class TopDragHotspot : public QWidget {
    Q_OBJECT
public:
    explicit TopDragHotspot(int height = 6, QWidget *parent = nullptr);

signals:
    void requestShowPanel();               // 拖拽到热区，要求下拉主面板
    void droppedUrls(const QList<QUrl>&);  // 用户在热区放开文件，转发 urls

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int m_height;
};

#endif // TOPDRAGHOTSPOT_H
