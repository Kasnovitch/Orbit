#include "src/services/projects/ProjectService.hpp"

#include <QSqlQuery>
#include <QVariant>

QVector<ProjectRef> ProjectService::listActive()
{
    QVector<ProjectRef> projects;

    QSqlQuery query;
    if (!query.exec("select id, name from projects where status = 'active' order by name"))
        return projects;

    while (query.next()) {
        ProjectRef project;
        project.id = query.value(0).toLongLong();
        project.name = query.value(1).toString();
        projects.append(project);
    }
    return projects;
}

qint64 ProjectService::ensureGeneralProject(qint64 ownerId)
{
    QSqlQuery find;
    if (find.exec("select id from projects where status = 'active' order by id limit 1")
        && find.next())
        return find.value(0).toLongLong();

    QSqlQuery insert;
    insert.prepare(
        "insert into projects (name, description, created_by) "
        "values (?, ?, ?) returning id");
    insert.addBindValue("General");
    insert.addBindValue("Default project until projects arrive.");
    insert.addBindValue(ownerId > 0 ? QVariant(ownerId) : QVariant());

    if (!insert.exec() || !insert.next())
        return 0;
    return insert.value(0).toLongLong();
}
