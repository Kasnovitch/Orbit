#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

struct CountdownRow {
    qint64 id = 0;
    qint64 projectId = 0; // 0 = global countdown (no project)
    QString title;
    QString description;
    QDateTime dueTime;
};

// day 5: countdowns crud. project_id is optional (global countdowns
// allowed), lists always come back nearest deadline first
class CountdownService
{
public:
    // global countdowns plus the ones belonging to the project, due_time asc
    QVector<CountdownRow> listForProject(qint64 projectId);

    // projectId 0 = global
    qint64 create(qint64 projectId, const QString& title, const QString& description,
                  const QDateTime& dueTime, qint64 createdBy);

    bool update(qint64 id, const QString& title, const QString& description,
                const QDateTime& dueTime);

    bool remove(qint64 id);

    // dashboard tiles: "active" = not past due yet
    int countUpcoming(qint64 projectId);
    int countPast(qint64 projectId);
};
