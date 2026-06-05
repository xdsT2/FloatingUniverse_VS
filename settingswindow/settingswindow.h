#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include "defines.h"
#include "panel/mainwindow.h"
#include "panel/floatball.h"
#include "panel/universepanel.h"
#include "settingscontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsWindow; }
QT_END_NAMESPACE

class QListWidgetItem;

class SettingsWindow : public QMainWindow
{
    Q_OBJECT
public:
    SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow() override;

private slots:
    void trayAction(QSystemTrayIcon::ActivationReason reason);

private:
    void initView();
    void initTray();
    void initKey();
    void initPanel();

    QRect screenGeometry() const;

public slots:
    void togglePanelVisibility();

protected:
    void showEvent(QShowEvent* e) override;
    void closeEvent(QCloseEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void returnToPrevWindow();
    void applyWindowTheme();
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    Ui::SettingsWindow *ui;
#ifdef Q_OS_WIN32
    HWND prevWindow = nullptr;
#endif
    MainWindow* panel = nullptr;
    FloatBall* floatBall = nullptr;
    QSystemTrayIcon* tray = nullptr;

    // 窗口拖拽
    bool m_dragging = false;
    QPoint m_dragPosition;
};
#endif // SETTINGSWINDOW_H
