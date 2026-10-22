#pragma once

#include <QDateTime>
#include <QHash>
#include <QString>
#include <QVariant>

// day 8: tiny in-memory ttl cache, everything on the ui thread. never
// holds passwords or session tokens - postgres stays the source of truth
// and countdown remaining time is always computed from the real due_time
class Cache
{
public:
    static Cache& instance();

    // expired entries count as a miss and get dropped
    QVariant get(const QString& key);
    void set(const QString& key, const QVariant& value, int ttlSeconds);

    // drops every entry whose key starts with the prefix (e.g. "tasks.")
    void invalidatePrefix(const QString& prefix);
    void clear();

private:
    Cache() = default;

    struct Entry {
        QVariant value;
        QDateTime expiresAt;
    };
    QHash<QString, Entry> m_entries;
};
