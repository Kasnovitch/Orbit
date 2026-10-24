#include "src/services/notifications/NotificationService.hpp"

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSqlQuery>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVariant>

#include "src/services/settings/SettingsService.hpp"

namespace {

// flat teal rounded square with a white O - drawn at runtime so there's
// no image asset to ship
QIcon buildTrayIcon()
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#14b8a6"));
    painter.drawRoundedRect(4, 4, 56, 56, 16, 16);

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(38);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "O");

    return QIcon(pixmap);
}

// keep the dedupe set from growing forever on a long-running session
const int maxSeenKeys = 400;

}

NotificationService& NotificationService::instance()
{
    static NotificationService service;
    return service;
}

NotificationService::NotificationService()
{
    m_timer = new QTimer(this);
    m_timer->setInterval(60 * 1000);
    connect(m_timer, &QTimer::timeout, this, &NotificationService::tick);
}

void NotificationService::attachWindow(QWidget* window)
{
    m_window = window;
}

void NotificationService::start(qint64 userId)
{
    m_userId = userId;
    m_seen.clear();
    syncTrayVisibility();
    m_timer->start();
    tick();
}

void NotificationService::stop()
{
    m_timer->stop();
    m_userId = 0;
    m_seen.clear();
    if (m_tray)
        m_tray->hide();
}

bool NotificationService::canHideToTray() const
{
    return m_tray && m_tray->isVisible() && m_window;
}

void NotificationService::showWindow()
{
    if (!m_window)
        return;
    m_window->show();
    m_window->raise();
    m_window->activateWindow();
}

void NotificationService::ensureTray()
{
    if (m_tray)
        return;

    m_tray = new QSystemTrayIcon(buildTrayIcon(), this);
    m_tray->setToolTip("Orbit");

    m_trayMenu = new QMenu();
    auto* openAction = m_trayMenu->addAction("Open Orbit");
    m_trayMenu->addSeparator();
    auto* quitAction = m_trayMenu->addAction("Quit");

    m_tray->setContextMenu(m_trayMenu);

    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger
                    || reason == QSystemTrayIcon::DoubleClick)
                    showWindow();
            });
    connect(openAction, &QAction::triggered, this,
            &NotificationService::showWindow);
    connect(quitAction, &QAction::triggered, this,
            &NotificationService::quitRequested);
}

void NotificationService::syncTrayVisibility()
{
    // the tray earns its place when either notifications or minimize-to-
    // tray is on; otherwise stay out of the way completely
    const SettingsRow& settings = SettingsService::currentSettings();
    const bool wanted = m_userId > 0
        && (settings.notificationsEnabled || settings.minimizeToTray);

    if (!wanted) {
        if (m_tray)
            m_tray->hide();
        return;
    }

    ensureTray();
    if (!m_tray->isVisible())
        m_tray->show();
}

void NotificationService::tick()
{
    if (m_userId <= 0)
        return;

    // settings may have changed since the last minute - re-evaluate
    syncTrayVisibility();

    const SettingsRow& settings = SettingsService::currentSettings();
    if (!settings.notificationsEnabled || !m_tray || !m_tray->isVisible())
        return;

    if (m_seen.size() > maxSeenKeys)
        m_seen.clear();

    const int lead = qMax(1, settings.notifyMinutesBefore);

    // countdowns coming up inside the lead window; paused ones stay on
    // the list but don't nag until they're resumed
    {
        QSqlQuery query;
        query.prepare(
            "select id, title, due_time from countdowns "
            "where due_time > now() and due_time <= now() + (? * interval '1 minute') "
            "and status <> 'paused'");
        query.addBindValue(lead);

        if (query.exec()) {
            while (query.next()) {
                const qint64 id = query.value(0).toLongLong();
                const QString title = query.value(1).toString();
                const QDateTime due = query.value(2).toDateTime().toUTC();
                const QString key = QStringLiteral("cd:%1:%2")
                                        .arg(id)
                                        .arg(due.toMSecsSinceEpoch());

                const qint64 minutes = QDateTime::currentDateTimeUtc().secsTo(due) / 60;
                const QString body = minutes < 1
                    ? QStringLiteral("due now \u2014 %1").arg(title)
                    : QStringLiteral("due in %1 min \u2014 %2").arg(minutes + 1).arg(title);

                if (!m_seen.contains(key)) {
                    m_seen.insert(key);
                    m_tray->showMessage("Orbit countdown", body,
                                        m_tray->icon(), 8000);
                }
            }
        }
    }

    // tasks due soon or already overdue (still open)
    {
        QSqlQuery query;
        query.prepare(
            "select id, title, due_time from tasks "
            "where status <> 'completed' and due_time is not null "
            "and due_time <= now() + (? * interval '1 minute')");
        query.addBindValue(lead);

        if (query.exec()) {
            while (query.next()) {
                const qint64 id = query.value(0).toLongLong();
                const QString title = query.value(1).toString();
                const QDateTime due = query.value(2).toDateTime().toUTC();
                const QString key = QStringLiteral("task:%1:%2")
                                        .arg(id)
                                        .arg(due.toMSecsSinceEpoch());

                const QString body = due < QDateTime::currentDateTimeUtc()
                    ? QStringLiteral("overdue \u2014 %1").arg(title)
                    : QStringLiteral("due soon \u2014 %1").arg(title);

                if (!m_seen.contains(key)) {
                    m_seen.insert(key);
                    m_tray->showMessage("Orbit task", body,
                                        m_tray->icon(), 8000);
                }
            }
        }
    }
}
