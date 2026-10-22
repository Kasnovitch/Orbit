#pragma once

#include <QDateTime>
#include <QString>

struct SettingsRow {
    qint64 userId = 0;
    bool notificationsEnabled = true;
    QString theme = "dark";     // dark / light / system
    QString timezone = "UTC";   // UTC, UTC+2, UTC-5 ...
};

// day 7: the settings page reads and writes the settings table. the row
// for the signed-in user is also kept in memory so date formatting can
// use the timezone everywhere without passing it around
class SettingsService
{
public:
    static SettingsRow get(qint64 userId);
    static bool save(qint64 userId, const SettingsRow& settings);

    // loads (or defaults) the signed-in user's row into the static copy
    static void loadCurrent(qint64 userId);
    static const SettingsRow& currentSettings();

    // "UTC+2" -> 120, "UTC-5" -> -300, "UTC" -> 0
    static int timezoneOffsetMinutes(const QString& timezone);

    // formats a utc time in the user's preferred timezone
    static QString formatInTimezone(const QDateTime& utcTime, const QString& format);
};
