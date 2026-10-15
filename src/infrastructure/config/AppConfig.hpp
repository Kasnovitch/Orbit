#pragma once

#include <QString>

struct DatabaseConfig
{
    QString host = "127.0.0.1";
    int port = 5432;
    QString name = "asalancountdown";
    QString user = "asalan";
    QString password;
};

class AppConfig
{
public:
    bool load();

    QString error() const { return m_error; }
    const DatabaseConfig& database() const { return m_database; }

private:
    DatabaseConfig m_database;
    QString m_error;
};
