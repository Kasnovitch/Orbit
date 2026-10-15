#include "src/infrastructure/config/AppConfig.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

bool AppConfig::load()
{
    const QStringList candidates = {
        "config/dev/AppConfig.json",
        "../config/dev/AppConfig.json",
        "../../config/dev/AppConfig.json",
        "../../../config/dev/AppConfig.json",
    };

    const QString appDir = QCoreApplication::applicationDirPath();

    QFile file;
    for (const QString& path : candidates) {
        if (QFile::exists(path)) {
            file.setFileName(path);
            break;
        }
        if (QFile::exists(appDir + "/" + path)) {
            file.setFileName(appDir + "/" + path);
            break;
        }
    }

    if (file.fileName().isEmpty()) {
        m_error = "config file not found (looked for config/dev/AppConfig.json)";
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_error = "could not open " + file.fileName();
        return false;
    }

    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonObject db = root.value("database").toObject();

    m_database.host = db.value("host").toString(m_database.host);
    m_database.port = db.value("port").toInt(m_database.port);
    m_database.name = db.value("name").toString(m_database.name);
    m_database.user = db.value("user").toString(m_database.user);
    m_database.password = db.value("password").toString();

    return true;
}
