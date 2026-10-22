#include "src/services/tasks/TaskService.hpp"

#include <QSqlQuery>
#include <QVariant>

namespace {

int countTasks(qint64 projectId, const QString& whereExtra,
               const QVariant& extraBind = QVariant())
{
    if (projectId <= 0)
        return 0;

    QSqlQuery query;
    query.prepare("select count(*) from tasks where project_id = ? and " + whereExtra);
    query.addBindValue(projectId);
    if (extraBind.isValid())
        query.addBindValue(extraBind);

    if (!query.exec() || !query.next())
        return 0;
    return query.value(0).toInt();
}

}

QVector<TaskRow> TaskService::listForProject(qint64 projectId)
{
    QVector<TaskRow> tasks;
    if (projectId <= 0)
        return tasks;

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

    return update.exec() && update.numRowsAffected() > 0;
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

    return update.exec() && update.numRowsAffected() > 0;
}

bool TaskService::remove(qint64 id)
{
    QSqlQuery del;
    del.prepare("delete from tasks where id = ?");
    del.addBindValue(id);
    return del.exec() && del.numRowsAffected() > 0;
}

int TaskService::countActive(qint64 projectId)
{
    return countTasks(projectId, "status <> 'completed'");
}

int TaskService::countCompleted(qint64 projectId)
{
    return countTasks(projectId, "status = 'completed'");
}

int TaskService::countDueSoon(qint64 projectId, int withinDays)
{
    return countTasks(projectId,
                      "status <> 'completed' and due_time > now() "
                      "and due_time <= now() + make_interval(days => ?)",
                      withinDays);
}
