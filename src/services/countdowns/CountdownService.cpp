#include "src/services/countdowns/CountdownService.hpp"

#include <QSqlQuery>
#include <QVariant>

#include "src/cache/Cache.hpp"

namespace {

const int cacheTtlSeconds = 30;
const int countdownRowsMeta = qRegisterMetaType<QVector<CountdownRow>>();

int countDue(qint64 projectId, bool upcoming)
{
    if (projectId <= 0)
        return 0;

    const QString cacheKey = QStringLiteral("countdowns.counts.%1.%2")
                                 .arg(projectId)
                                 .arg(upcoming ? "upcoming" : "past");

    if (const QVariant cached = Cache::instance().get(cacheKey); cached.isValid())
        return cached.toInt();

    QSqlQuery query;
    query.prepare(QString(
        "select count(*) from countdowns "
        "where (project_id is null or project_id = ?) and due_time %1 now()")
                      .arg(upcoming ? ">" : "<="));
    query.addBindValue(projectId);

    if (!query.exec() || !query.next())
        return 0;

    const int count = query.value(0).toInt();
    Cache::instance().set(cacheKey, count, cacheTtlSeconds);
    return count;
}

}

QVector<CountdownRow> CountdownService::listForProject(qint64 projectId)
{
    QVector<CountdownRow> countdowns;
    if (projectId <= 0)
        return countdowns;

    const QString cacheKey = QStringLiteral("countdowns.list.%1").arg(projectId);
    if (const QVariant cached = Cache::instance().get(cacheKey); cached.isValid())
        return cached.value<QVector<CountdownRow>>();

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

    Cache::instance().set(cacheKey, QVariant::fromValue(countdowns), cacheTtlSeconds);
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

    Cache::instance().invalidatePrefix("countdowns.");
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

    if (!update.exec() || update.numRowsAffected() == 0)
        return false;

    Cache::instance().invalidatePrefix("countdowns.");
    return true;
}

bool CountdownService::remove(qint64 id)
{
    QSqlQuery del;
    del.prepare("delete from countdowns where id = ?");
    del.addBindValue(id);

    if (!del.exec() || del.numRowsAffected() == 0)
        return false;

    Cache::instance().invalidatePrefix("countdowns.");
    return true;
}

int CountdownService::countUpcoming(qint64 projectId)
{
    return countDue(projectId, true);
}

int CountdownService::countPast(qint64 projectId)
{
    return countDue(projectId, false);
}
