#pragma once

#include <QString>

struct AuthUser;

// day 3: who may modify a project. owner (created_by) and admins bypass
// the table entirely; everyone else needs a permissions row with
// can_write. sharing UI comes later - the service is ready for it
class PermissionService
{
public:
    bool canModify(const AuthUser& user, qint64 projectId);

    // grant/revoke for the (future) share dialog; both no-ops for
    // owners/admins' own projects are the caller's problem, not ours
    bool grant(qint64 userId, qint64 projectId, bool canWrite);
    bool revoke(qint64 userId, qint64 projectId);
};
