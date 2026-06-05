#ifndef UNIVERSEPANEL_H
#define UNIVERSEPANEL_H

#include <QObject>
#include <QWidget>
#include "facilemenu.h"
#include "myjson.h"
#include "icontextitem.h"
#include "longtextitem.h"
#include "imageitem.h"
#include "carditem.h"
#include "todoitem.h"

class MainWindow; // 前向声明

#define eachitem(x) for (int i = items.size() - 1; i >= 0; i--)\
{\
    auto item = items.at(i);\
    x\
}

class UniversePanel : public QWidget
{
    Q_OBJECT
    friend class MainWindow;
public:
    explicit UniversePanel(QWidget *parent = nullptr);
    ~UniversePanel() override;
    
    // Public API for drag-out support
    void moveSelectedItems(QPoint delta);
    bool isItemDragging() const { return isDragging; }
    QList<QUrl> getSelectedFileUrls() const; // 获取选中项的文件URL列表

private:
    void initPanel();
    void initAction();
    void readItems();
    IconTextItem *createLinkItem(QPoint pos, bool center, const QIcon& icon, const QString& text, const QString& link, PanelItemType type);
    IconTextItem *createLinkItem(QPoint pos, bool center, const QString& iconName, const QString& text, const QString& link, PanelItemType type);
    LongTextItem *createTextItem(QPoint pos, const QString& text, bool enableHtml);
    ImageItem *createImageItem(QPoint pos, const QPixmap& pixmap);
    ImageItem *createImageItem(QPoint pos, const QString& image);
    CardItem *createCardItem(QPoint pos);
    TodoItem *createTodoItem(QPoint pos);
    void connectItem(PanelItemBase* item);
    void deleteItem(PanelItemBase* item);
    bool isMouseInPanel() const;
    bool hasItemUsing() const;

signals:
    void openSettings();
    void signalExpandPanel();
    void signalFoldPanel();
    void signalSetKeepFix(bool keep);
    void signalToggleFixed();  // 右键菜单切换固定状态
    void signalKeepPanelState(FuncType func);
    void itemDragStarted();     // items拖拽开始
    void itemDragEnded();       // items拖拽结束

public slots:
    void handleDragFinished();

    void saveLater();
    void save();
    void selectAll(bool containIgnored = true);
    void unselectAll();
    void selectItem(PanelItemBase* item, const QPoint& pos = UNDEFINED_POS);
    void unselectItem(PanelItemBase* item);
    void triggerItem(PanelItemBase* item);
    void raiseItem(PanelItemBase* item);
    void lowerItem(PanelItemBase* item);

private slots:
    void startDragSelectedItems();
    void pasteFromClipboard(QPoint pos);
    void insertMimeData(const QMimeData *mime, QPoint pos);

public:
    bool getWebPageNameAndIcon(QString url, QString& pageName, QPixmap &pageIcon);

protected:
    void focusOutEvent(QFocusEvent *event) override;

    void paintEvent(QPaintEvent *) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void contextMenuEvent(QContextMenuEvent *event) override;
    void showAddMenu(FacileMenu* addMenu);
    void addPastAction(FacileMenu* menu, QPoint pos, bool split = false);
    void keyPressEvent(QKeyEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    bool pressing = false; // 左键按下
    QPoint pressPos;
    bool moving = false; // 正在移动items
    bool scening = false; // 正在移动画面
    QPoint draggingPos;
    FacileMenu* currentMenu = nullptr;
    bool _block_menu = false;
    bool _release_outter = false; // 鼠标松开的时候，是不是在外面
    QTimer* saveTimer;
    QTimer* keepTopTimer;

    QList<PanelItemBase*> items;
    QSet<PanelItemBase*> selectedItems;
    
    // 拖拽时保存原始位置（用于拖出安全框时恢复）
    QMap<PanelItemBase*, QPoint> originalPositions;
    bool isDragging = false; // 是否正在拖拽
    
    // 安全框相关：更新安全框状态
    void updateSafetyFrameState();
};

#endif // UNIVERSEPANEL_H
