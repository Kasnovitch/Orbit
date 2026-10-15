#pragma once

#include <QString>

struct DatabaseConfig;

class Database
{
public:
    bool open(const DatabaseConfig& cfg);
    bool ensureTables();

    QString status() const { return m_status; }

private:
    QString m_status;
};
