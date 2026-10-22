#include "src/cache/Cache.hpp"

Cache& Cache::instance()
{
    static Cache cache;
    return cache;
}

QVariant Cache::get(const QString& key)
{
    auto it = m_entries.find(key);
    if (it == m_entries.end())
        return QVariant();

    if (QDateTime::currentDateTimeUtc() >= it->expiresAt) {
        m_entries.erase(it);
        return QVariant();
    }
    return it->value;
}

void Cache::set(const QString& key, const QVariant& value, int ttlSeconds)
{
    Entry entry;
    entry.value = value;
    entry.expiresAt = QDateTime::currentDateTimeUtc().addSecs(ttlSeconds);
    m_entries.insert(key, entry);
}

void Cache::invalidatePrefix(const QString& prefix)
{
    for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (it.key().startsWith(prefix))
            it = m_entries.erase(it);
        else
            ++it;
    }
}

void Cache::clear()
{
    m_entries.clear();
}
