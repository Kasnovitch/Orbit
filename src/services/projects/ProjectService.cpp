#include "src/services/projects/ProjectService.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "src/cache/Cache.hpp"
#include "src/data/auth/AuthService.hpp"
#include "src/services/permissions/PermissionService.hpp"

namespace {
const int cacheTtlSeconds = 30;
const int projectRowsMeta = qRegisterMetaType<QVector<ProjectRef>>();

// shared guard for every mutation: permission check + a stable error
// message when it fails
bool guardModify(qint64 projectId, const AuthUser& by, QString* error)
{
    PermissionService permissions;
    if (!permissions.canModify(by, projectId)) {
        if (error)
            *error = "you don't have permission to change this project.";
        return false;
    }
    return true;
}

bool setArchiveState(qint64 projectId, const AuthUser& by, bool archived,
                     QString* error)
{
    if (!guardModify(projectId, by, error))
        return false;

    QSqlQuery query;
    query.prepare(
        "update projects set status = ?, "
        "archived_at = case when ? then now() else null end, "
        "updated_at = now() where id = ?");
    query.addBindValue(archived ? "archived" : "active");
    query.addBindValue(archived);
    query.addBindValue(projectId);

    if (!query.exec() || query.numRowsAffected() == 0) {
        if (error)
            *error = "could not save the project: " + query.lastError().text();
        return false;
    }

    Cache::instance().invalidatePrefix("projects.");
    return true;
}

}

QVector<ProjectRef> ProjectService::listActive()
{
    const QString cacheKey = QStringLiteral("projects.list");
    if (const QVariant cached = Cache::instance().get(cacheKey); cached.isValid())
        return cached.value<QVector<ProjectRef>>();

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

    Cache::instance().set(cacheKey, QVariant::fromValue(projects), cacheTtlSeconds);
    return projects;
}

QVector<ProjectRow> ProjectService::listAll()
{
    QVector<ProjectRow> projects;

    QSqlQuery query;
    if (!query.exec(
            "select p.id, p.name, p.description, p.status, p.created_by, "
            "coalesce(u.username, ''), p.created_date "
            "from projects p left join users u on u.id = p.created_by "
            "order by p.status, p.name"))
        return projects;

    while (query.next()) {
        ProjectRow project;
        project.id = query.value(0).toLongLong();
        project.name = query.value(1).toString();
        project.description = query.value(2).toString();
        project.status = query.value(3).toString();
        project.createdBy = query.value(4).toLongLong();
        project.createdByName = query.value(5).toString();
        project.createdDate = query.value(6).toDateTime();
        projects.append(project);
    }

    return projects;
}

qint64 ProjectService::create(const QString& name, const QString& description,
                              qint64 createdBy, QString* error)
{
    const QString cleanName = name.trimmed();
    if (cleanName.isEmpty()) {
        if (error)
            *error = "the project needs a name.";
        return 0;
    }

    QSqlQuery insert;
    insert.prepare(
        "insert into projects (name, description, created_by) "
        "values (?, ?, ?) returning id");
    insert.addBindValue(cleanName);
    insert.addBindValue(description.trimmed());
    insert.addBindValue(createdBy > 0 ? QVariant(createdBy) : QVariant());

    if (!insert.exec() || !insert.next()) {
        if (error)
            *error = "could not create the project: " + insert.lastError().text();
        return 0;
    }

    Cache::instance().invalidatePrefix("projects.");
    return insert.value(0).toLongLong();
}

bool ProjectService::update(qint64 projectId, const QString& name,
                            const QString& description, const AuthUser& by,
                            QString* error)
{
    const QString cleanName = name.trimmed();
    if (cleanName.isEmpty()) {
        if (error)
            *error = "the project needs a name.";
        return false;
    }
    if (!guardModify(projectId, by, error))
        return false;

    QSqlQuery query;
    query.prepare(
        "update projects set name = ?, description = ?, updated_at = now() "
        "where id = ?");
    query.addBindValue(cleanName);
    query.addBindValue(description.trimmed());
    query.addBindValue(projectId);

    if (!query.exec() || query.numRowsAffected() == 0) {
        if (error)
            *error = "could not save the project: " + query.lastError().text();
        return false;
    }

    Cache::instance().invalidatePrefix("projects.");
    return true;
}

bool ProjectService::archive(qint64 projectId, const AuthUser& by, QString* error)
{
    return setArchiveState(projectId, by, true, error);
}

bool ProjectService::unarchive(qint64 projectId, const AuthUser& by, QString* error)
{
    return setArchiveState(projectId, by, false, error);
}

bool ProjectService::remove(qint64 projectId, const AuthUser& by, QString* error)
{
    if (!guardModify(projectId, by, error))
        return false;

    QSqlQuery query;
    query.prepare("delete from projects where id = ?");
    query.addBindValue(projectId);

    if (!query.exec() || query.numRowsAffected() == 0) {
        if (error)
            *error = "could not delete the project: " + query.lastError().text();
        return false;
    }

    Cache::instance().invalidatePrefix("projects.");
    return true;
}

qint64 ProjectService::ensureGeneralProject(qint64 ownerId)
{
    QSqlQuery find;
    if (find.exec("select id from projects where status = 'active' order by id limit 1")
        && find.next())
        return find.value(0).toLongLong();

    QString error;
    const qint64 id = create("General", "Default project.", ownerId, &error);
    return id;
}
