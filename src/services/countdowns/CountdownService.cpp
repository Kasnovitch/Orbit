#include "src/services/countdowns/CountdownService.hpp"

#include <QSqlQuery>
#include <QVariant>

namespace {

int countDue(qint64 projectId, bool upcoming)
{
    if (projectId <= 0)
        return 0;

    QSqlQuery query;
    query.prepare(QString(
        "select count(*) from countdowns "
        "where (project_id is null or project_id = ?) and due_time %1 now()")
                      .arg(upcoming ? ">" : "<="));
    query.addBindValue(projectId);

    if (!query.exec() || !query.next())
        return 0;
    return query.value(0).toInt();
}

}

QVector<CountdownRow> CountdownService::listForProject(qint64 projectId)
{
    QVector<CountdownRow> countdowns;
    if (projectId <= 0)
        return countdowns;

    QSqlQuery query;
    query.prepare(
        "select id, project_id, title, description, due_time "
        "from countdowns where project_id is null or project_id = ? "
        "order by due_time asc");
    query.addBindValue(projectId);

    if (!query.exec())
        return countdowns;

    while (query.next()) {
        CountdownRow row;
        row.id = query.value(0).toLongLong();
        row.projectId = query.value(1).toLongLong();
        row.title = query.value(2).toString();
        row.description = query.value(3).toString();
        row.dueTime = query.value(4).toDateTime();
        countdowns.append(row);
    }
    return countdowns;
}

qint64 CountdownService::create(qint64 projectId, const QString& title,
                                const QString& description, const QDateTime& dueTime,
                                qint64 createdBy)
{
    if (title.trimmed().isEmpty() || !dueTime.isValid())
        return 0;

    QSqlQuery insert;
    insert.prepare(
        "insert into countdowns (project_id, title, description, due_time, created_by) "
        "values (?, ?, ?, ?, ?) returning id");
    insert.addBindValue(projectId > 0 ? QVariant(projectId) : QVariant());
    insert.addBindValue(title.trimmed());
    insert.addBindValue(description);
    insert.addBindValue(dueTime);
    insert.addBindValue(createdBy > 0 ? QVariant(createdBy) : QVariant());

    if (!insert.exec() || !insert.next())
        return 0;
    return insert.value(0).toLongLong();
}

bool CountdownService::update(qint64 id, const QString& title,
                              const QString& description, const QDateTime& dueTime)
{
    if (id <= 0 || title.trimmed().isEmpty() || !dueTime.isValid())
        return false;

    QSqlQuery update;
    update.prepare(
        "update countdowns set title = ?, description = ?, due_time = ?, "
        "updated_at = now() where id = ?");
    update.addBindValue(title.trimmed());
    update.addBindValue(description);
    update.addBindValue(dueTime);
    update.addBindValue(id);

    return update.exec() && update.numRowsAffected() > 0;
}

bool CountdownService::remove(qint64 id)
{
    QSqlQuery del;
    del.prepare("delete from countdowns where id = ?");
    del.addBindValue(id);
    return del.exec() && del.numRowsAffected() > 0;
}

int CountdownService::countUpcoming(qint64 projectId)
{
    return countDue(projectId, true);
}

int CountdownService::countPast(qint64 projectId)
{
    return countDue(projectId, false);
}
