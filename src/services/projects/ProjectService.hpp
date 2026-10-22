#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVector>

struct AuthUser;

struct ProjectRef {
    qint64 id = 0;
    QString name;
};

struct ProjectRow {
    qint64 id = 0;
    QString name;
    QString description;
    QString status; // active | archived
    qint64 createdBy = 0;
    QString createdByName;
    QDateTime createdDate;
};

Q_DECLARE_METATYPE(QVector<ProjectRef>)

// projects crud with the day 3 permission model: creator and admins can
// modify, everyone else needs a permissions row (see PermissionService).
// the selector list is cached (day 8) and invalidated on changes
class ProjectService
{
public:
    QVector<ProjectRef> listActive();

    // for the Projects page: every project, archived included
    QVector<ProjectRow> listAll();

    // returns the new id, or 0 (error filled). name is required
    qint64 create(const QString& name, const QString& description,
                  qint64 createdBy, QString* error);

    bool update(qint64 projectId, const QString& name,
                const QString& description, const AuthUser& by, QString* error);

    // archive/unarchive flips status + archived_at
    bool archive(qint64 projectId, const AuthUser& by, QString* error);
    bool unarchive(qint64 projectId, const AuthUser& by, QString* error);

    // hard delete; children go with it (tasks/countdowns/links/files
    // cascade in the database - confirmed in the UI prompt)
    bool remove(qint64 projectId, const AuthUser& by, QString* error);

    // one default project so the selector never starts empty
    qint64 ensureGeneralProject(qint64 ownerId);
};
