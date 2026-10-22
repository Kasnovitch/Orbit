#include "src/services/tasks/TaskService.hpp"

#include <QSqlQuery>
#include <QVariant>

#include "src/cache/Cache.hpp"

namespace {

const int cacheTtlSeconds = 30;
const int taskRowsMeta = qRegisterMetaType<QVector<TaskRow>>();

int countTasks(qint64 projectId, const QString& cacheKey,
               const QString& whereExtra, const QVariant& extraBind = QVariant())
{
    if (projectId <= 0)
        return 0;

    if (const QVariant cached = Cache::instance().get(cacheKey); cached.isValid())
        return cached.toInt();

    QSqlQuery query;
    query.prepare("select count(*) from tasks where project_id = ? and " + whereExtra);
    query.addBindValue(projectId);
    if (extraBind.isValid())
        query.addBindValue(extraBind);

    if (!query.exec() || !query.next())
        return 0;

    const int count = query.value(0).toInt();
    Cache::instance().set(cacheKey, count, cacheTtlSeconds);
    return count;
}

}

QVector<TaskRow> TaskService::listForProject(qint64 projectId)
{
    QVector<TaskRow> tasks;
    if (projectId <= 0)
        return tasks;

    const QString cacheKey = QStringLiteral("tasks.list.%1").arg(projectId);
    if (const QVariant cached = Cache::instance().get(cacheKey); cached.isValid())
        return cached.value<QVector<TaskRow>>();

    QSqlQuery query;
    query.prepare(
        "select id, title, contents, status, priority, due_time "
        "from tasks where project_id = ? "
        "order by case status when 'completed' then 1 else 0 end, "
        "due_time nulls last, id");
    query.addBindValue(projectId);

    if (!query.exec())
        return tasks;

    while (query.next()) {
        TaskRow task;
        task.id = query.value(0).toLongLong();
        task.title = query.value(1).toString();
        task.contents = query.value(2).toString();
        task.status = query.value(3).toString();
        task.priority = query.value(4).toString();
        task.dueTime = query.value(5).toDateTime();
        tasks.append(task);
    }

    Cache::instance().set(cacheKey, QVariant::fromValue(tasks), cacheTtlSeconds);
    return tasks;
}

qint64 TaskService::create(qint64 projectId, const QString& title,
                           const QString& contents, const QString& priority,
                           const QDateTime& dueTime, qint64 createdBy)
{
    if (projectId <= 0 || title.trimmed().isEmpty())
        return 0;

    QSqlQuery insert;
    insert.prepare(
        "insert into tasks (project_id, title, contents, priority, due_time, created_by) "
        "values (?, ?, ?, ?, ?, ?) returning id");
    insert.addBindValue(projectId);
    insert.addBindValue(title.trimmed());
    insert.addBindValue(contents);
    insert.addBindValue(priority);
    insert.addBindValue(dueTime.isValid() ? QVariant(dueTime) : QVariant());
    insert.addBindValue(createdBy > 0 ? QVariant(createdBy) : QVariant());

    if (!insert.exec() || !insert.next())
        return 0;

    Cache::instance().invalidatePrefix("tasks.");
    return insert.value(0).toLongLong();
}

bool TaskService::update(qint64 id, const QString& title, const QString& contents,
                         const QString& priority, const QDateTime& dueTime)
{
    if (id <= 0 || title.trimmed().isEmpty())
        return false;

    QSqlQuery update;
    update.prepare(
        "update tasks set title = ?, contents = ?, priority = ?, due_time = ?, "
        "updated_at = now() where id = ?");
    update.addBindValue(title.trimmed());
    update.addBindValue(contents);
    update.addBindValue(priority);
    update.addBindValue(dueTime.isValid() ? QVariant(dueTime) : QVariant());
    update.addBindValue(id);

    if (!update.exec() || update.numRowsAffected() == 0)
        return false;

    Cache::instance().invalidatePrefix("tasks.");
    return true;
}

bool TaskService::setStatus(qint64 id, const QString& status)
{
    if (id <= 0)
        return false;

    const bool completed = status == "completed";

    QSqlQuery update;
    update.prepare(
        "update tasks set status = ?, completed_at = ?, updated_at = now() "
        "where id = ?");
    update.addBindValue(status);
    update.addBindValue(completed ? QVariant(QDateTime::currentDateTimeUtc()) : QVariant());
    update.addBindValue(id);

    if (!update.exec() || update.numRowsAffected() == 0)
        return false;

    Cache::instance().invalidatePrefix("tasks.");
    return true;
}

bool TaskService::remove(qint64 id)
{
    QSqlQuery del;
    del.prepare("delete from tasks where id = ?");
    del.addBindValue(id);

    if (!del.exec() || del.numRowsAffected() == 0)
        return false;

    Cache::instance().invalidatePrefix("tasks.");
    return true;
}

int TaskService::countActive(qint64 projectId)
{
    return countTasks(projectId,
                      QStringLiteral("tasks.counts.%1.active").arg(projectId),
                      "status <> 'completed'");
}

int TaskService::countCompleted(qint64 projectId)
{
    return countTasks(projectId,
                      QStringLiteral("tasks.counts.%1.completed").arg(projectId),
                      "status = 'completed'");
}

int TaskService::countDueSoon(qint64 projectId, int withinDays)
{
    return countTasks(projectId,
                      QStringLiteral("tasks.counts.%1.duesoon").arg(projectId),
                      "status <> 'completed' and due_time > now() "
                      "and due_time <= now() + make_interval(days => ?)",
                      withinDays);
}
