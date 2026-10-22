#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

struct ProjectRef {
    qint64 id = 0;
    QString name;
};

Q_DECLARE_METATYPE(QVector<ProjectRef>)

// day 4 needs two things from projects: the list for the selector and a
// default project to work in. day 3 extends this with the rest of the
// crud. the list is cached (day 8) and invalidated on changes
class ProjectService
{
public:
    QVector<ProjectRef> listActive();

    // temporary bootstrap while projects crud isn't built: makes sure there
    // is at least one active project to put tasks/countdowns/links/files in.
    // returns its id, or 0 on failure
    qint64 ensureGeneralProject(qint64 ownerId);
};
