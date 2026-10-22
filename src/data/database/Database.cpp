#include "src/data/database/Database.hpp"

#include <cstddef>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "src/infrastructure/config/AppConfig.hpp"

namespace {

// the tables day 1 needs. each new day adds its own here; existing
// tables are never dropped or recreated.
struct Table {
    const char* name;
    const char* ddl;
};

bool applyTables(const Table* tables, std::size_t count, int& created, QString& status)
{
    for (std::size_t i = 0; i < count; ++i) {
        // bail early if the table is already there, otherwise we'd
        // try to remake an existing database on every launch
        QSqlQuery exists;
        exists.prepare("select to_regclass(?)");
        exists.addBindValue(QString("public.%1").arg(tables[i].name));

        if (!exists.exec() || !exists.next()) {
            status = "database: " + exists.lastError().text();
            return false;
        }
        if (!exists.value(0).isNull())
            continue;

        QSqlQuery create;
        if (!create.exec(QString::fromLatin1(tables[i].ddl))) {
            status = "database: " + create.lastError().text();
            return false;
        }
        ++created;
    }
    return true;
}

// same treatment for the indexes: statements look like
// "create index <name> on ..." so we can pull the name out and check it
bool applyIndexes(const char* const* indexes, std::size_t count, QString& status)
{
    for (std::size_t i = 0; i < count; ++i) {
        const QString statement = QString::fromLatin1(indexes[i]);
        const QString indexName = statement.section(' ', 2, 2);

        QSqlQuery exists;
        exists.prepare("select to_regclass(?)");
        exists.addBindValue("public." + indexName);

        if (!exists.exec() || !exists.next()) {
            status = "database: " + exists.lastError().text();
            return false;
        }
        if (!exists.value(0).isNull())
            continue;

        QSqlQuery create;
        if (!create.exec(statement)) {
            status = "database: " + create.lastError().text();
            return false;
        }
    }
    return true;
}

const Table day1Tables[] = {
    { "users", R"(
create table users (
    id bigserial primary key,
    username text not null,
    password_hash text not null,
    role text not null default 'user',
    email text default '',
    display_name text default '',
    is_active boolean not null default true,
    last_login_at timestamptz,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now(),
    constraint users_username_unique unique (username)
)
)" },
    { "projects", R"(
create table projects (
    id bigserial primary key,
    name text not null,
    description text default '',
    created_by bigint references users (id) on delete set null,
    status text not null default 'active',
    visibility text not null default 'private',
    created_date timestamptz not null default now(),
    updated_at timestamptz not null default now(),
    archived_at timestamptz
)
)" },
    { "tasks", R"(
create table tasks (
    id bigserial primary key,
    project_id bigint not null references projects (id) on delete cascade,
    title text not null,
    contents text default '',
    status text not null default 'todo',
    priority text not null default 'normal',
    due_time timestamptz,
    created_by bigint references users (id) on delete set null,
    assigned_to bigint references users (id) on delete set null,
    created_date timestamptz not null default now(),
    updated_at timestamptz not null default now(),
    completed_at timestamptz
)
)" },
};

// day 5: countdowns. project_id stays nullable for global countdowns
const Table day5Tables[] = {
    { "countdowns", R"(
create table countdowns (
    id bigserial primary key,
    project_id bigint references projects (id) on delete cascade,
    title text not null,
    description text default '',
    due_time timestamptz not null,
    created_by bigint references users (id) on delete set null,
    created_date timestamptz not null default now(),
    updated_at timestamptz not null default now()
)
)" },
};

// day 6: links + files. both must belong to a project
const Table day6Tables[] = {
    { "project_links", R"(
create table project_links (
    id bigserial primary key,
    project_id bigint not null references projects (id) on delete cascade,
    title text not null,
    url text not null,
    created_by bigint references users (id) on delete set null,
    created_date timestamptz not null default now(),
    updated_at timestamptz not null default now()
)
)" },
    { "project_files", R"(
create table project_files (
    id bigserial primary key,
    project_id bigint not null references projects (id) on delete cascade,
    filename text not null,
    storage_key text not null,
    mime_type text default '',
    size_bytes bigint not null default 0,
    uploaded_by bigint references users (id) on delete set null,
    created_date timestamptz not null default now(),
    updated_at timestamptz not null default now()
)
)" },
};

const char* day1Indexes[] = {
    "create index projects_created_by_idx on projects (created_by)",
    "create index tasks_project_id_idx on tasks (project_id)",
    "create index tasks_due_time_idx on tasks (due_time)",
    "create index tasks_created_by_idx on tasks (created_by)",
    "create index tasks_assigned_to_idx on tasks (assigned_to)",
};

const char* day5Indexes[] = {
    "create index countdowns_project_id_idx on countdowns (project_id)",
    "create index countdowns_due_time_idx on countdowns (due_time)",
    "create index countdowns_created_by_idx on countdowns (created_by)",
};

const char* day6Indexes[] = {
    "create index project_links_project_id_idx on project_links (project_id)",
    "create index project_files_project_id_idx on project_files (project_id)",
};

}

bool Database::open(const DatabaseConfig& cfg)
{
    // only ever one connection for the whole app - if it's already
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
    int created = 0;

    if (!applyTables(day1Tables, std::size(day1Tables), created, m_status)
        || !applyTables(day5Tables, std::size(day5Tables), created, m_status)
        || !applyTables(day6Tables, std::size(day6Tables), created, m_status)
        || !applyIndexes(day1Indexes, std::size(day1Indexes), m_status)
        || !applyIndexes(day5Indexes, std::size(day5Indexes), m_status)
        || !applyIndexes(day6Indexes, std::size(day6Indexes), m_status))
        return false;

    if (created > 0)
        m_status += " (" + QString::number(created) + " tables created)";
    return true;
}
