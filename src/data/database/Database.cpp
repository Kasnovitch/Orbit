#include "src/data/database/Database.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "src/infrastructure/config/AppConfig.hpp"

namespace {
const char* schemaCandidates[] = {
    "db/schema/Schema.sql",
    "../db/schema/Schema.sql",
    "../../db/schema/Schema.sql",
    "../../../db/schema/Schema.sql",
};
}

bool Database::open(const DatabaseConfig& cfg)
{
    // only ever one connection for the whole app — if it's already
    // registered we just reuse it instead of opening a second one
    QSqlDatabase db;
    if (QSqlDatabase::contains()) {
        db = QSqlDatabase::database();
        if (db.isOpen()) {
            m_status = "database: connected to " + cfg.name + " @ " + cfg.host;
            return true;
        }
    } else {
        db = QSqlDatabase::addDatabase("QPSQL");
    }

    db.setHostName(cfg.host);
    db.setPort(cfg.port);
    db.setDatabaseName(cfg.name);
    db.setUserName(cfg.user);
    db.setPassword(cfg.password);

    if (!db.open()) {
        m_status = "database: " + db.lastError().text();
        return false;
    }

    m_status = "database: connected to " + cfg.name + " @ " + cfg.host;
    return true;
}

bool Database::ensureTables()
{
    QSqlQuery check;
    if (!check.exec("select to_regclass('public.users')") || !check.next()) {
        m_status = "database: " + check.lastError().text();
        return false;
    }

    // table already exists, nothing to do
    if (!check.value(0).isNull())
        return true;

    QFile file;
    const QString appDir = QCoreApplication::applicationDirPath();
    for (const char* path : schemaCandidates) {
        if (QFile::exists(path)) {
            file.setFileName(path);
            break;
        }
        if (QFile::exists(appDir + "/" + path)) {
            file.setFileName(appDir + "/" + path);
            break;
        }
    }

    if (file.fileName().isEmpty() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_status = "database: schema file not found";
        return false;
    }

    // drop comment lines then run each statement
    QString cleaned;
    const QStringList lines = QString::fromUtf8(file.readAll()).split('\n');
    for (const QString& line : lines) {
        if (line.trimmed().startsWith("--"))
            continue;
        cleaned += line + "\n";
    }

    const QStringList statements = cleaned.split(';', Qt::SkipEmptyParts);
    for (const QString& statement : statements) {
        if (statement.trimmed().isEmpty())
            continue;

        QSqlQuery query;
        if (!query.exec(statement)) {
            m_status = "database: " + query.lastError().text();
            return false;
        }
    }

    m_status += " (tables created)";
    return true;
}
