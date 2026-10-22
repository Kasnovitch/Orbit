#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

struct TaskRow {
    qint64 id = 0;
    QString title;
    QString contents;
    QString status;   // todo / in_progress / completed (spec values)
    QString priority; // low / normal / high
    QDateTime dueTime; // invalid = no due date
};

// day 4: tasks crud, always tied to a project (project_id is required)
class TaskService
{
public:
    // completed tasks last, then by due time
    QVector<TaskRow> listForProject(qint64 projectId);

    qint64 create(qint64 projectId, const QString& title, const QString& contents,
                  const QString& priority, const QDateTime& dueTime, qint64 createdBy);

    bool update(qint64 id, const QString& title, const QString& contents,
                const QString& priority, const QDateTime& dueTime);

    // completed_at is set/cleared here so it always matches the status
    bool setStatus(qint64 id, const QString& status);

    bool remove(qint64 id);

    // dashboard tiles, scoped to one project
    int countActive(qint64 projectId);
    int countCompleted(qint64 projectId);
    int countDueSoon(qint64 projectId, int withinDays);
};
