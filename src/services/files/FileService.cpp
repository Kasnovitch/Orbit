#include "src/services/files/FileService.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

#include "src/cache/Cache.hpp"

namespace {
const int cacheTtlSeconds = 30;
const int fileRowsMeta = qRegisterMetaType<QVector<FileRow>>();
}

QVector<FileRow> FileService::listForProject(qint64 projectId)
{
    QVector<FileRow> files;
    if (projectId <= 0)
        return files;

    const QString cacheKey = QStringLiteral("files.list.%1").arg(projectId);
    if (const QVariant cached = Cache::instance().get(cacheKey); cached.isValid())
        return cached.value<QVector<FileRow>>();

    QSqlQuery query;
    query.prepare(
        "select id, filename, storage_key, mime_type, size_bytes "
        "from project_files where project_id = ? order by id");
    query.addBindValue(projectId);

    if (!query.exec())
        return files;

    while (query.next()) {
        FileRow row;
        row.id = query.value(0).toLongLong();
        row.filename = query.value(1).toString();
        row.storageKey = query.value(2).toString();
        row.mimeType = query.value(3).toString();
        row.sizeBytes = query.value(4).toLongLong();
        files.append(row);
    }

    Cache::instance().set(cacheKey, QVariant::fromValue(files), cacheTtlSeconds);
    return files;
}

QString FileService::upload(const QString& sourcePath, qint64 projectId,
                            qint64 uploaderId, QString* error)
{
    if (projectId <= 0) {
        if (error)
            *error = "pick a project first.";
        return QString();
    }

    const QFileInfo source(sourcePath);
    if (!source.exists() || !source.isFile()) {
        if (error)
            *error = "that file doesn't exist.";
        return QString();
    }
    if (source.size() <= 0) {
        if (error)
            *error = "that file is empty.";
        return QString();
    }
    if (source.size() > maxUploadBytes) {
        if (error)
            *error = "files are capped at 100 MB.";
        return QString();
    }

    // storage key is a fresh uuid so uploaded names never collide or
    // leak anything about the user's disk
    const QString storageKey =
        QUuid::createUuid().toString(QUuid::WithoutBraces);

    QDir().mkpath(storageRoot());
    const QString destination = storageRoot() + "/" + storageKey;

    if (!QFile::copy(sourcePath, destination)) {
        if (error)
            *error = "could not copy the file into storage.";
        return QString();
    }

    const QString mime = QMimeDatabase().mimeTypeForFile(source).name();

    QSqlQuery insert;
    insert.prepare(
        "insert into project_files (project_id, filename, storage_key, mime_type, "
        "size_bytes, uploaded_by) values (?, ?, ?, ?, ?, ?) returning id");
    insert.addBindValue(projectId);
    insert.addBindValue(source.fileName());
    insert.addBindValue(storageKey);
    insert.addBindValue(mime);
    insert.addBindValue(source.size());
    insert.addBindValue(uploaderId > 0 ? QVariant(uploaderId) : QVariant());

    if (!insert.exec() || !insert.next()) {
        QFile::remove(destination);
        if (error)
            *error = "could not record the file.";
        return QString();
    }

    Cache::instance().invalidatePrefix("files.");
    return source.fileName();
}

bool FileService::remove(qint64 id, QString* error)
{
    QSqlQuery find;
    find.prepare("select storage_key from project_files where id = ?");
    find.addBindValue(id);
    if (!find.exec() || !find.next()) {
        if (error)
            *error = "that file no longer exists.";
        return false;
    }
    const QString storageKey = find.value(0).toString();

    QSqlQuery del;
    del.prepare("delete from project_files where id = ?");
    del.addBindValue(id);
    if (!del.exec() || del.numRowsAffected() == 0) {
        if (error)
            *error = "could not remove the file record.";
        return false;
    }

    // the row is gone; losing the disk file now is only untidy
    QFile::remove(storageRoot() + "/" + storageKey);
    Cache::instance().invalidatePrefix("files.");
    return true;
}

QString FileService::absolutePath(const QString& storageKey) const
{
    return storageRoot() + "/" + storageKey;
}

QString FileService::storageRoot() const
{
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base + "/files";
}
