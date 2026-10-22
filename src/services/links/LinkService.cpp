#include "src/services/links/LinkService.hpp"

#include <QSqlQuery>
#include <QUrl>
#include <QVariant>

QVector<LinkRow> LinkService::listForProject(qint64 projectId)
{
    QVector<LinkRow> links;
    if (projectId <= 0)
        return links;

    QSqlQuery query;
    query.prepare(
        "select id, project_id, title, url from project_links "
        "where project_id = ? order by id");
    query.addBindValue(projectId);

    if (!query.exec())
        return links;

    while (query.next()) {
        LinkRow link;
        link.id = query.value(0).toLongLong();
        link.projectId = query.value(1).toLongLong();
        link.title = query.value(2).toString();
        link.url = query.value(3).toString();
        links.append(link);
    }
    return links;
}

bool LinkService::add(qint64 projectId, const QString& title, const QString& url,
                      qint64 createdBy, QString* error)
{
    if (projectId <= 0) {
        if (error)
            *error = "pick a project first.";
        return false;
    }
    if (title.trimmed().isEmpty()) {
        if (error)
            *error = "the link needs a title.";
        return false;
    }

    const QString urlError = validateUrl(url);
    if (!urlError.isEmpty()) {
        if (error)
            *error = urlError;
        return false;
    }

    QSqlQuery insert;
    insert.prepare(
        "insert into project_links (project_id, title, url, created_by) "
        "values (?, ?, ?, ?) returning id");
    insert.addBindValue(projectId);
    insert.addBindValue(title.trimmed());
    insert.addBindValue(url.trimmed());
    insert.addBindValue(createdBy > 0 ? QVariant(createdBy) : QVariant());

    if (!insert.exec() || !insert.next()) {
        if (error)
            *error = "could not save the link.";
        return false;
    }
    return true;
}

bool LinkService::update(qint64 id, const QString& title, const QString& url,
                         QString* error)
{
    if (id <= 0 || title.trimmed().isEmpty()) {
        if (error)
            *error = "the link needs a title.";
        return false;
    }

    const QString urlError = validateUrl(url);
    if (!urlError.isEmpty()) {
        if (error)
            *error = urlError;
        return false;
    }

    QSqlQuery update;
    update.prepare(
        "update project_links set title = ?, url = ?, updated_at = now() "
        "where id = ?");
    update.addBindValue(title.trimmed());
    update.addBindValue(url.trimmed());
    update.addBindValue(id);

    return update.exec() && update.numRowsAffected() > 0;
}

bool LinkService::remove(qint64 id)
{
    QSqlQuery del;
    del.prepare("delete from project_links where id = ?");
    del.addBindValue(id);
    return del.exec() && del.numRowsAffected() > 0;
}

QString LinkService::validateUrl(const QString& url)
{
    const QUrl parsed(url.trimmed());
    if (!parsed.isValid() || parsed.scheme().isEmpty() || parsed.host().isEmpty())
        return "that doesn't look like a valid url.";
    if (parsed.scheme() != "http" && parsed.scheme() != "https")
        return "only http and https urls are allowed.";
    return QString();
}
