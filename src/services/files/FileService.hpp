#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

struct FileRow {
    qint64 id = 0;
    QString filename; // original name, what the user sees
    QString storageKey;
    QString mimeType;
    qint64 sizeBytes = 0;
};

Q_DECLARE_METATYPE(QVector<FileRow>)

// day 6: project files. the bytes live on disk under the app's data
// folder; the database only stores the row (storage_key points at the
// file). lists are cached (day 8), uploads/removes invalidate them
class FileService
{
public:
    static constexpr qint64 maxUploadBytes = 100 * 1024 * 1024; // 100 MB

    QVector<FileRow> listForProject(qint64 projectId);

    // copies the file into storage and records the row. fills error on
    // failure, returns the new filename or an empty string
    QString upload(const QString& sourcePath, qint64 projectId, qint64 uploaderId,
                   QString* error);

    // removes the row and the stored bytes
    bool remove(qint64 id, QString* error);

    // full path for a storage key (for opening the file)
    QString absolutePath(const QString& storageKey) const;

private:
    QString storageRoot() const;
};
