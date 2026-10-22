#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

struct LinkRow {
    qint64 id = 0;
    qint64 projectId = 0;
    QString title;
    QString url;
};

Q_DECLARE_METATYPE(QVector<LinkRow>)

// day 6: project links. urls are validated here, not in the ui. lists go
// through the day 8 cache, mutations invalidate them
class LinkService
{
public:
    QVector<LinkRow> listForProject(qint64 projectId);

    // fills error with a human message when validation or the insert fails
    bool add(qint64 projectId, const QString& title, const QString& url,
             qint64 createdBy, QString* error);

    bool update(qint64 id, const QString& title, const QString& url, QString* error);

    bool remove(qint64 id);

    // empty string = valid, otherwise the reason it isn't
    static QString validateUrl(const QString& url);
};
