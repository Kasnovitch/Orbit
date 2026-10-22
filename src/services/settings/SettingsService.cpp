#include "src/services/settings/SettingsService.hpp"

#include <QSqlQuery>
#include <QVariant>

#include "src/services/audit/AuditService.hpp"

namespace {
SettingsRow s_current;
}

SettingsRow SettingsService::get(qint64 userId)
{
    SettingsRow settings;
    settings.userId = userId;
    if (userId <= 0)
        return settings;

    QSqlQuery query;
    query.prepare(
        "select notifications_enabled, theme, timezone, "
        "always_on_top, minimize_to_tray, notify_minutes_before "
        "from settings where user_id = ?");
    query.addBindValue(userId);

    if (!query.exec() || !query.next())
        return settings; // no row yet - the defaults stand in

    settings.notificationsEnabled = query.value(0).toBool();
    settings.theme = query.value(1).toString();
    settings.timezone = query.value(2).toString();
    settings.alwaysOnTop = query.value(3).toBool();
    settings.minimizeToTray = query.value(4).toBool();
    settings.notifyMinutesBefore = query.value(5).toInt();
    return settings;
}

bool SettingsService::save(qint64 userId, const SettingsRow& settings)
{
    if (userId <= 0)
        return false;

    QSqlQuery upsert;
    upsert.prepare(
        "insert into settings (user_id, notifications_enabled, theme, timezone, "
        "always_on_top, minimize_to_tray, notify_minutes_before) "
        "values (?, ?, ?, ?, ?, ?, ?) "
        "on conflict (user_id) do update set "
        "notifications_enabled = excluded.notifications_enabled, "
        "theme = excluded.theme, "
        "timezone = excluded.timezone, "
        "always_on_top = excluded.always_on_top, "
        "minimize_to_tray = excluded.minimize_to_tray, "
        "notify_minutes_before = excluded.notify_minutes_before, "
        "updated_at = now()");
    upsert.addBindValue(userId);
    upsert.addBindValue(settings.notificationsEnabled);
    upsert.addBindValue(settings.theme);
    upsert.addBindValue(settings.timezone);
    upsert.addBindValue(settings.alwaysOnTop);
    upsert.addBindValue(settings.minimizeToTray);
    upsert.addBindValue(settings.notifyMinutesBefore);

    if (!upsert.exec())
        return false;

    AuditService::log(userId, "account.settings_change", "user", userId,
                      settings.theme + "/" + settings.timezone
                          + (settings.alwaysOnTop ? " ontop" : "")
                          + (settings.minimizeToTray ? " tray" : ""));
    return true;
}

void SettingsService::loadCurrent(qint64 userId)
{
    s_current = get(userId);
}

const SettingsRow& SettingsService::currentSettings()
{
    return s_current;
}

int SettingsService::timezoneOffsetMinutes(const QString& timezone)
{
    // "UTC", "UTC+2", "UTC-5", optionally with minutes like "UTC+5:30"
    if (timezone == "UTC" || timezone.isEmpty())
        return 0;

    const QString suffix = timezone.mid(3); // after "UTC"
    if (suffix.isEmpty())
        return 0;

    bool negative = false;
    QString body = suffix;
    if (body.startsWith('-')) {
        negative = true;
        body = body.mid(1);
    } else if (body.startsWith('+')) {
        body = body.mid(1);
    }

    const QStringList parts = body.split(':');
    int minutes = parts.value(0).toInt() * 60;
    if (parts.size() > 1)
        minutes += parts.value(1).toInt();
    return negative ? -minutes : minutes;
}

QString SettingsService::formatInTimezone(const QDateTime& utcTime, const QString& format)
{
    if (!utcTime.isValid())
        return QString();

    const int offsetSeconds = timezoneOffsetMinutes(s_current.timezone) * 60;
    return utcTime.toUTC().toOffsetFromUtc(offsetSeconds).toString(format);
}
