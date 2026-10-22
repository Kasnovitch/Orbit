#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

struct AuditRow {
    qint64 id = 0;
    qint64 userId = 0;
    QString username;
    QString action;
    QString entityType;
    qint64 entityId = 0;
    QString metadata;
    QDateTime createdAt;
};

// day 9: append-only trail of account and project changes (spec: project
// create/delete, permission changes, account changes). no secrets ever
// go into metadata - plain text facts only
class AuditService
{
public:
    // fire and forget: auditing must never break the action it records
    static void log(qint64 userId, const QString& action,
                    const QString& entityType = QString(), qint64 entityId = 0,
                    const QString& metadata = QString());

    static QVector<AuditRow> recent(int limit = 30);
};
