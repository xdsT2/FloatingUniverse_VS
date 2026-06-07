#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QUrl>
#include <QTimer>
#include "facilemenu.h"

class UniversePanel;

class MainWindow : public QWidget
{
    Q_OBJECT
    friend class UniversePanel;
public:
    explicit MainWindow(QWidget *parent = nullptr);

    void initPanel();

signals:
    void openSettings();
    void minimizeToBall();
    void panelPositionChanged();
    void safetyFrameStateChanged(bool canDragOut);

public slots:
    void expandPanel();
    void foldPanel();
    void keepPanelState(FuncType func);
    void setPanelFixed(bool f);
    QRect screenGeometry() const;
    void showPanel();
    void detectAndApplySystemTheme();
    int getSafetyMargin() const { return safetyMargin; }
    void handleDroppedUrls(const QList<QUrl>& urls);
    bool isPanelFixed() const { return fixing; }
    bool isAnimating() const { return animating; }
    bool isFolded() const { return m_isFolded; }
    void setDragHotspot(class TopDragHotspot* hotspot) { m_dragHotspot = hotspot; }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

private:
    UniversePanel* panel = nullptr;
    QWidget* titleBar = nullptr;
    QLabel* titleLabel = nullptr;
    QPushButton* dragHintBtn = nullptr;
    QPushButton* floatBtn = nullptr;
    QPushButton* settingsBtn = nullptr;

    bool expanding = false;
    bool animating = false;
    bool fixing = false;
    int boundaryWidth = 8;
    QPixmap panelBlurPixmap;

    bool _prev_fixing = false;
    
    // Drag support
    bool dragging = false;
    QPoint dragPos;
    
    // Title bar drag
    bool titleDragging = false;
    QPoint titleDragPos;
    bool titleDragged = false; // Track if title was dragged to prevent button click
    
    // Safety frame
    int safetyMargin = 20; // 安全框内边距（像素）
    bool canDragOut = false; // 是否允许拖出到外部
    bool safetyFrameVisible = false; // 安全框是否可见（点击触发后保持显示）

    // Floating state
    QRect m_lastFloatingGeometry; // 保存最后一次悬浮时的 geometry
    bool m_wasFloating = false; // 之前是否处于悬浮状态

    // Drag hotspot
    TopDragHotspot* m_dragHotspot = nullptr;

    // 折叠状态标记
    bool m_isFolded = false;

    // 展开后短暂忽略 resize-start 的时间窗口
    bool m_ignoreResizeStart = false;
    QTimer m_ignoreResizeTimer;

private:
    void initTitleBar();
    QRect getSafetyFrameRect() const;
    bool isItemOutsideSafetyFrame(const QRect& itemRect) const;
};

#endif // FLOATPANEL_H
