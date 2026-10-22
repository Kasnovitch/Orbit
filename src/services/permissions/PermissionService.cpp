#include "src/services/permissions/PermissionService.hpp"

#include <QSqlQuery>
#include <QVariant>

#include "src/data/auth/AuthService.hpp"

bool PermissionService::canModify(const AuthUser& user, qint64 projectId)
{
    if (user.id <= 0 || projectId <= 0)
        return false;

    // admins can touch anything
    if (user.role == "admin")
        return true;

    // the creator keeps full control
    QSqlQuery owner;
    owner.prepare("select created_by from projects where id = ?");
    owner.addBindValue(projectId);
    if (owner.exec() && owner.next()
        && owner.value(0).toLongLong() == user.id)
        return true;

    // everyone else needs an explicit grant
    QSqlQuery grant;
    grant.prepare(
        "select can_write from permissions where user_id = ? and project_id = ?");
    grant.addBindValue(user.id);
    grant.addBindValue(projectId);
    return grant.exec() && grant.next() && grant.value(0).toBool();
}

bool PermissionService::grant(qint64 userId, qint64 projectId, bool canWrite)
{
    if (userId <= 0 || projectId <= 0)
        return false;

    QSqlQuery query;
    query.prepare(
        "insert into permissions (user_id, project_id, can_write) "
        "values (?, ?, ?) "
        "on conflict (user_id, project_id) "
        "do update set can_write = excluded.can_write");
    query.addBindValue(userId);
    query.addBindValue(projectId);
    query.addBindValue(canWrite);
    return query.exec();
}

bool PermissionService::revoke(qint64 userId, qint64 projectId)
{
    if (userId <= 0 || projectId <= 0)
        return false;

    QSqlQuery query;
    query.prepare(
        "delete from permissions where user_id = ? and project_id = ?");
    query.addBindValue(userId);
    query.addBindValue(projectId);
    return query.exec();
}
