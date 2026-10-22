#include "src/services/audit/AuditService.hpp"

#include <QSqlQuery>
#include <QVariant>

void AuditService::log(qint64 userId, const QString& action,
                       const QString& entityType, qint64 entityId,
                       const QString& metadata)
{
    QSqlQuery query;
    query.prepare(
        "insert into audit_logs (user_id, action, entity_type, entity_id, metadata) "
        "values (?, ?, ?, ?, ?)");
    query.addBindValue(userId > 0 ? QVariant(userId) : QVariant());
    query.addBindValue(action);
    query.addBindValue(entityType);
    query.addBindValue(entityId > 0 ? QVariant(entityId) : QVariant());
    query.addBindValue(metadata);
    query.exec();
}

QVector<AuditRow> AuditService::recent(int limit)
{
    QVector<AuditRow> rows;

    QSqlQuery query;
    query.prepare(
        "select a.id, coalesce(a.user_id, 0), coalesce(u.username, '?'), "
        "a.action, a.entity_type, coalesce(a.entity_id, 0), a.metadata, a.created_at "
        "from audit_logs a left join users u on u.id = a.user_id "
        "order by a.id desc limit ?");
    query.addBindValue(limit);

    if (!query.exec())
        return rows;

    while (query.next()) {
        AuditRow row;
        row.id = query.value(0).toLongLong();
        row.userId = query.value(1).toLongLong();
        row.username = query.value(2).toString();
        row.action = query.value(3).toString();
        row.entityType = query.value(4).toString();
        row.entityId = query.value(5).toLongLong();
        row.metadata = query.value(6).toString();
        row.createdAt = query.value(7).toDateTime();
        rows.append(row);
    }

    return rows;
}
