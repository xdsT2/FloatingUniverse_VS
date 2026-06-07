#ifndef ICONTEXTITEM_H
#define ICONTEXTITEM_H

#include "panelitembase.h"
#include <QEnterEvent>
#include <QTimer>

#define FILE_PREFIX QString("_local:///")

// 长文件名滚动标签
class MarqueeLabel : public QLabel
{
    Q_OBJECT
public:
    explicit MarqueeLabel(QWidget *parent = nullptr);
    void setFullText(const QString &txt);

protected:
    void paintEvent(QPaintEvent* e) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateScrollingNeed();
    int m_offset = 0;
    QTimer *m_timer = nullptr;
    int m_speedPxPerSec = 50;
    bool m_scrollingNeeded = false;
    bool m_isHovering = false;
};

class IconTextItem : public PanelItemBase
{
    Q_OBJECT
public:
    IconTextItem(QWidget* parent);

    virtual MyJson toJson() const override;
    virtual void fromJson(const MyJson& json) override;
    virtual void initResource() override;
    virtual void releaseResource() override;

    void setIcon(const QString& iconName);
    void setText(const QString& text);
    void setLink(const QString& link);
    void setFastOpen(bool fast);
    void setOpenDirLevel(int level);
    void updateTextColor();
    void applyIconSize();

    QString getText() const;
    QString getIconName() const;
    QString getLink() const;
    QStringList getRealLinks() const;
    bool isFastOpen() const;

    static QString saveIconFile(const QIcon& icon);
    static QString saveIconFile(const QPixmap& pixmap);

    virtual void shake(int range = 5);
    virtual void nod(int range = 5);
    virtual void jump(int range = 10);
    virtual void shrink(int range = 10);
    virtual void jitter(QPoint startPos, int range = 10);

protected:
    virtual void facileMenuEvent(FacileMenu *menu) override;
    virtual void triggerEvent() override;
    virtual bool canDropEvent(const QMimeData *mime) override;
    void paintEvent(QPaintEvent *ev) override;
    virtual bool startNativeFileDrag() override; // 启动原生文件拖放
    void dropEvent(QDropEvent *event) override;
    virtual void hoverEvent(const QPoint &startPos) override;
    void enterEvent(QEnterEvent* event) override;

private:
    void showFacileDir(QString path, FacileMenu *parentMenu, int level);

private:
    QLabel* iconLabel;
    MarqueeLabel* textLabel;  // 使用MarqueeLabel支持长文本滚动

    QString iconName; // 图标文件名（包括后缀）
    QString text; // 显示的标题
    QString link; // 文件或者网址

    bool hideAfterTrigger = true;
    bool fastOpen = false; // 左键快速打开
    int openLevel = 3; // 打开的级别，文件多的时候越大越慢

    bool fileMissing = false; // 文件是否已从磁盘删除或移动

    bool _shaking = false;
    bool _nodding = false;
    bool _jumping = false;
    bool _shrinking = false;
    bool _jittering = false;
};

#endif // ICONTEXTITEM_H
