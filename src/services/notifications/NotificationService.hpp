#pragma once

#include <QObject>
#include <QSet>

class QString;
class QMenu;
class QSystemTrayIcon;
class QTimer;
class QWidget;

// day 10: system tray + desktop notifications, the piece the settings
// toggle was waiting for. owns the tray icon (open/quit menu), hides the
// main window to it when "minimize to tray" is on, and pops a balloon
// when a countdown is due within the user's lead time or a task is
// due/overdue. each item only notifies once per due time
class NotificationService : public QObject
{
    Q_OBJECT
public:
    static NotificationService& instance();

    // called on login/logout; window is what the tray restores
    void attachWindow(QWidget* window);
    void start(qint64 userId);
    void stop();

    // true when closing the window can hide to the tray instead
    bool canHideToTray() const;

    void showWindow();

signals:
    void quitRequested();

private:
    NotificationService();

    void ensureTray();
    void syncTrayVisibility();
    void tick();

    QWidget* m_window = nullptr;
    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_trayMenu = nullptr;
    QTimer* m_timer = nullptr;
    QSet<QString> m_seen; // "kind:id:due" keys already notified this run
    qint64 m_userId = 0;
};
