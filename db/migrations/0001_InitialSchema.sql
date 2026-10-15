-- migration 0001: initial schema
-- the full schema lives in db/schema/Schema.sql and the app runs it on
-- first startup. this file just records what the first version was.

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
