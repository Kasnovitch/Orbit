-- asalancountdown - database schema
-- the app runs this on first startup if the tables don't exist yet.
-- keep statements simple, one semicolon per statement (the app splits on ';').

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
);

create table sessions (
    id bigserial primary key,
    user_id bigint not null references users (id) on delete cascade,
    token_hash text not null,
    created_at timestamptz not null default now(),
    last_activity_at timestamptz not null default now(),
    expires_at timestamptz not null,
    absolute_expires_at timestamptz not null,
    revoked_at timestamptz,
    user_agent text default '',
    ip_address text default ''
);

create index sessions_user_id_idx on sessions (user_id);
create index sessions_token_hash_idx on sessions (token_hash);

create table settings (
    user_id bigint primary key references users (id) on delete cascade,
    notifications_enabled boolean not null default true,
    theme text not null default 'system',
    timezone text not null default 'system',
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create table teams (
    id bigserial primary key,
    name text not null,
    description text default '',
    created_by bigint references users (id) on delete set null,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create table projects (
    id bigserial primary key,
    name text not null,
    description text default '',
    created_by bigint references users (id) on delete set null,
    status text not null default 'active',
    visibility text not null default 'private',
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now(),
    archived_at timestamptz
);

create index projects_created_by_idx on projects (created_by);

create table project_members (
    id bigserial primary key,
    project_id bigint not null references projects (id) on delete cascade,
    user_id bigint not null references users (id) on delete cascade,
    role text not null default 'member',
    joined_at timestamptz not null default now()
);

create index project_members_project_id_idx on project_members (project_id);
create index project_members_user_id_idx on project_members (user_id);

create table project_links (
    id bigserial primary key,
    project_id bigint not null references projects (id) on delete cascade,
    title text not null,
    url text not null,
    created_by bigint references users (id) on delete set null,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create index project_links_project_id_idx on project_links (project_id);

create table project_files (
    id bigserial primary key,
    project_id bigint not null references projects (id) on delete cascade,
    filename text not null,
    storage_key text not null,
    mime_type text default '',
    size_bytes bigint not null default 0,
    uploaded_by bigint references users (id) on delete set null,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create index project_files_project_id_idx on project_files (project_id);

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
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now(),
    completed_at timestamptz
);

create index tasks_project_id_idx on tasks (project_id);
create index tasks_due_time_idx on tasks (due_time);

create table countdowns (
    id bigserial primary key,
    project_id bigint references projects (id) on delete cascade,
    title text not null,
    description text default '',
    due_time timestamptz not null,
    created_by bigint references users (id) on delete set null,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now()
);

create index countdowns_project_id_idx on countdowns (project_id);
create index countdowns_due_time_idx on countdowns (due_time);

create table permissions (
    id bigserial primary key,
    role text not null,
    operation text not null,
    allowed boolean not null default true,
    created_at timestamptz not null default now(),
    constraint permissions_role_operation_unique unique (role, operation)
);

create table audit_logs (
    id bigserial primary key,
    user_id bigint references users (id) on delete set null,
    action text not null,
    entity_type text default '',
    entity_id bigint,
    metadata text default '',
    created_at timestamptz not null default now()
);

create index audit_logs_user_id_idx on audit_logs (user_id);
