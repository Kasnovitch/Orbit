# to do list

> complete as of day 12 - every box below is done. kept as the original
> checklist / spec reference; nothing here is still pending. the only
> open question from the notes (half-hour timezones) is still out of
> scope, everything else shipped.

## setup

- [x] set up the database files and get postgres connecting
- [x] decide what the app opens on — login/register first or straight to the dashboard
- [x] tables should create themselves once the database file exists (sql comes from the c++ side)
- [x] make sure the database doesnt remake itself constantly if it already exists
- [x] tasks table
- [x] add empty tiles for now so the dashboard isnt blank
- [x] hook the database up to the frontend for the tile logic/stats (placeholder for now)
- [x] task list should read from the database once its set up and show the list
- [x] utility classes

## database tables

- [x] users — username (unique), password hashed not plaintext, role user/admin, email, display name, is_active so admins can disable without deleting, last_login_at
- [x] countdowns — project_id (nullable for global countdowns), title, description, due_time, created_by
- [x] tasks — project_id always required, title, contents, status todo/in_progress/completed, priority, due_time optional, created_by, assigned_to (can be null), completed_at
- [x] projects — name, description, created_by (owner check), status active/archived, visibility, archived_at
- [x] project_links — must belong to a project, title + url, validate urls before saving
- [x] project_files — must belong to a project, filename, storage_key, mime type, size, uploaded_by. actual files live on disk, not in the database. validate uploads + permissions
- [x] sessions — server side, hashed token in an httponly cookie, inactivity expiry + absolute max, revoke on logout, cleanup old ones
- [x] settings — notifications on/off, theme dark/light/system, timezone
- [x] teams — just the table for now, feature stays off
- [x] project_members — table only, for later when we share projects
- [x] audit_logs — who did what and when. no passwords or tokens in here
- [x] permissions — permission records for the permission checks
- [x] foreign keys everywhere, bigints/uuids consistently, timestamptz, indexes on project_id, due_time, user_id
- [x] decide what happens to tasks/links/files when a project gets deleted
- [x] keep db credentials out of the code

## login + sessions

- [x] login page (and register if we go that route)
- [x] hash passwords properly
- [x] 30 min inactivity timeout, 7 day max session life
- [x] remember me option if we need it
- [x] logout kills the session straight away
- [x] changing password or disabling an account invalidates sessions
- [x] clean up expired sessions automatically
- [x] save last_login_at on successful login

## projects

- [x] projects crud with owner check (created_by)
- [x] current project selector — picks which tasks, countdowns, links and files show up
- [x] projects show their links
- [x] check who created the project

## tasks + countdowns

- [x] tasks crud tied to the selected project
- [x] statuses: todo, in_progress, completed (+ completed_at)
- [x] countdowns crud, project optional (global ones allowed)
- [x] countdown list sorted by due_time ascending, shortest one first with its title showing

## links + files

- [x] add/edit/remove project links, check the url is valid first
- [x] file upload with validation (type, size) and permission checks
- [x] separate panel/window on the main dashboard listing all files for the selected project

## settings page

- [x] settings page for user preferences + account stuff
- [x] change password from here (still hashed properly)
- [x] theme: dark / light / system
- [x] timezone affects how dates and times show
- [x] notifications toggle, uses the browser/system defaults. browser permission handled separately from the in-app setting

## caching

- [x] check cache first, if miss go to postgres, store result with a ttl
- [x] cache project lists/details, project links, task summaries, countdowns
- [x] invalidate when a project/task/countdown/link changes
- [x] never cache passwords or auth stuff, keep cached data scoped per user
- [x] countdown seconds always calculated from the real due_time, not cached
- [x] maybe redis later if we need shared caching

## permissions + security + audit

- [x] permission record for the role/operation. every operation goes through a permission check — owner gets all permissions, otherwise check the permission records for everyone else
- [x] permission checks happen server side, dont trust the frontend
- [x] validate all input on the server
- [x] use transactions when changing related records
- [x] audit log for project create/delete, permission changes, account changes
- [x] admins can manage users and system settings

## teams

- [x] placeholder only, leave it off until we actually need it

---

## dashboard checklist (what the main screen needs to do)

- [x] shortest countdown first, title visible
- [x] countdowns sorted by due_time ascending
- [x] projects show their links
- [x] separate panel for the selected project's files
- [x] current project selector
- [x] every task has a project_id
- [x] caching in place so we're not hammering the database
- [x] settings page
- [x] teams is just a placeholder

---

## notes / decisions

- postgres for both local dev and production, same engine either way
- foreign keys + transactions keep the related stuff consistent
- no mongo (data is relational), no sqlite for now (maybe later for offline), no redis for now
- jsonb is there if we want flexible metadata later
- session timeouts (30 min / 7 days) can be changed later
- implementation order: db tables → login/sessions → projects + selector → tasks + countdowns → sort countdowns → links + files → settings → caching → audit/security → teams stays a placeholder
- main goal: clean dashboard with login, projects, tasks, countdowns, files, settings and caching. keep it organised so we can add stuff later without rebuilding everything

---

# the actual spec (for reference)

## users

`users = id, username, password_hash, role, email, display_name, is_active, last_login_at, created_at, updated_at`

- roles: user and admin
- passwords securely hashed, never plaintext
- usernames unique, only one user per name
- admins manage users + system-wide settings
- is_active lets an admin disable an account without deleting it
- last_login_at = last successful login

## countdowns

`countdowns = id, project_id, title, description, due_time, created_by, created_date, updated_at`

- project_id can be null if we add global countdowns
- title identifies the countdown, description has extra info
- due_time is the deadline as date + time
- created_by = who made it
- show nearest deadline first
- updated_at = last edit

## tasks

`tasks = id, project_id, title, contents, status, priority, due_time, created_by, assigned_to, created_date, updated_at, completed_at`

- every task needs a valid project_id
- title = short name, contents = description
- status: todo / in_progress / completed
- priority for prioritising
- due_time can be null
- created_by = who made it, assigned_to can be null if unassigned
- completed_at = when it got finished, updated_at on every edit

## projects

`projects = id, name, description, created_by, status, visibility, created_date, updated_at, archived_at`

- name is the project name, description explains it
- created_by identifies who created it (owner check)
- status: active or archived, archived_at nullable
- read/write permission type, permission id, reference permissions
- projects display their associated links
- the current project selector decides which project's tasks/countdowns/links/files show
- include a check to see who created the project

## project_links

`project_links = id, project_id, title, url, created_by, created_date, updated_at`

- every link belongs to a project
- title = what's displayed, url = destination
- created_by = who added it
- validate URLs before saving

## project_files

`project_files = id, project_id, filename, storage_key, mime_type, size_bytes, uploaded_by, created_date, updated_at`

- every file belongs to a project
- all files for the selected project show in a separate window/panel on the main dashboard
- storage_key points at where the file actually lives
- real files go on the filesystem / object storage, not in the database
- validate uploads and check the user is allowed to access the files

## sessions

`sessions = id, user_id, token_hash, created_at, last_activity_at, expires_at, absolute_expires_at, revoked_at, user_agent, ip_address`

- persist across browser restarts when remember me is on
- stored server side
- secure random token in an httponly cookie, store the hash
- logout invalidates the session
- expire after inactivity, plus an absolute max lifetime
- invalidate after password change or account disable
- clean expired sessions up automatically

session expiry: 30 min inactivity, 7 day max, remember me if needed, re-login after expiry, logout invalidates immediately, values adjustable later.

## settings

`settings = user_id, notifications_enabled, theme, timezone, created_at, updated_at`

- notifications_enabled turns system/browser notifications on/off, use the browser defaults where supported
- change password from settings, still hashed
- theme: dark / light / system
- timezone controls how dates/times are displayed
- browser notification permission is handled separately from the app setting

## teams (placeholder)

`teams = id, name, description, created_by, created_date, updated_at`

- leave it inactive until we need it, just keep the table ready

## project_members (future)

`project_members = id, project_id, user_id, role, joined_at`

- sharing projects between users, project roles later
- only add this when we actually need multi-user collaboration

## audit_logs

`audit_logs = id, user_id, action, entity_type, entity_id, metadata, created_at`

- project created/deleted, permission changes, account changes
- shows who did what and when
- never store passwords, session tokens or other sensitive stuff in here

## database notes

- postgres is the main database, better suited since the tables are connected
- foreign keys between tasks, projects, countdowns, users, files
- transactions to keep related changes consistent
- no mongo needed, data is mostly relational
- jsonb available later for flexible metadata
- postgres locally for dev and for production so both behave the same
- sqlite maybe later for offline/standalone, dont run two live databases for the same data unless we really need sync

## caching notes

- reduces repeated queries, start with app-level caching, redis later if needed
- postgres stays the source of truth
- flow: check cache → hit? return it : fetch from postgres → store with ttl → invalidate on change
- cache project lists/details, links, task summaries, countdowns, other stuff that doesn't change often
- dont cache passwords or auth secrets
- be careful caching permissions/private project data, users must never see someone else's cached data
- invalidate when project/task/countdown/link changes
- countdown remaining time always comes from the real due_time
- start with short ttl's and tune later

## security + performance notes

- uuid or bigints for primary keys, consistently
- foreign keys on all the relationships
- indexes on project_id, due_time, user_id
- timestamptz for dates/times
- usernames unique
- validate all input server side
- check permissions server side, not just in the frontend
- transactions when modifying related records
- decide delete behaviour for tasks/links/files when a project is deleted
- db credentials and secret keys stay out of the source code
- permission record for the role/operation, every operation goes through a permission check, owner = all permissions, otherwise check everyone else's permission records




- Add local share (phone to pc on software.)
