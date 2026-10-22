# progress log

## day 1 - setup and shell (15 oct 2026)

### done

- git repo on branch `development`, .gitignore first (build output and
  `config/` with real credentials never get committed), config.example.json
  at the root, readme with build + qpsql notes
- cmake project renamed to orbit, builds with qt 6.11 + mingw on windows,
  produces `Orbit.exe` and `OrbitConsole.exe`
- postgres: fresh `orbit` database on 192.168.1.50 (owner `asalan`);
  `config/dev/AppConfig.json` points at it (local only, ignored)
- day 1 schema embedded in `src/data/database/Database.cpp`:
  `users`, `projects`, `tasks` + indexes on project_id/due_time/user refs.
  each table and index is checked with `to_regclass` and only created when
  missing - an existing database is never touched again
- primary keys: **bigint** (`bigserial`) everywhere, sticking with this
- app opens on the login screen, then the dashboard. day 1 login is a
  pass-through (any input enters); register/remember-me widgets are hidden
  until day 2
- sidebar has all 6 nav items now (Dashboard, Tasks, Countdowns, Projects,
  Teams, Settings); the four that aren't built yet open placeholder pages
- dashboard has the 5 stat cards from the reference (titles only, values are
  em-dashes until day 4)
- dark theme accent switched from blue to teal `#14b8a6`
- console renamed to orbit, banner kept, points at the same database

### next

day 2: login page (real this time), argon2id hashing via libsodium, sessions
+ settings tables, logout, expired-session cleanup, last_login_at,
remember-me storage.

### decisions

- opens on login -> dashboard (agreed before day 1 started); auth itself is
  day 2, so day 1 login just lets you through
- fresh `orbit` database; the old asalancountdown database is left alone
- schema lives in the c++ source (spec says embedded) - the old
  `db/schema/Schema.sql` + `db/migrations/` files were removed so there's one
  source of truth
- config loader stays in `src/infrastructure/config` (matches the scaffold
  layout that was already in the repo instead of moving things on day 1)
- `priority` stored as `text` for now - text vs small int is a day 4
  question, default is `'normal'`
- `created_date` on projects/tasks but `created_at` on users: follows the
  table list in the spec exactly
- `OrbitConsole` kept from the old codebase and renamed; it shares the db
  layer

### open questions

- `to-dolist.md` in the repo root is from the asalancountdown days - keep it
  or delete it?
- the empty scaffold files (`include/`, `tests/`, `src/services/`,
  `src/security/`, `src/cache/`, `src/domain/`, `src/common/` and the
  `docs/*.md` stubs) are placeholders with no content - they stay untouched
  until the day their module is needed, then they get wired into the build

## day 4 - tasks (16 oct 2026)

### done

- `TaskService`: list/create/update/status/delete for one project, plus the
  three tile counts (active, completed, due within 7 days)
- dashboard Tasks panel is real now: add bar, rows with a status combo
  (Not Started / In Progress / Completed - stored as todo / in_progress /
  completed), edit dialog (title, contents, priority low/normal/high, due
  date or none), delete. completed tasks sort to the bottom
- completing a task sets `completed_at`, un-completing clears it
- `ProjectSelector` reads active projects from the database now and emits
  the selected project id; the dashboard (all four panels + the tiles)
  follows it
- dashboard stat tiles: Active/Completed Tasks and Upcoming Deadlines show
  real numbers for the selected project; the two countdown tiles stay dashes
  until day 5
- temporary bootstrap in `Application`: seeds the default admin user and a
  "General" project so days 4-6 are usable before day 2 (real login) and
  day 3 (projects crud) land. the sidebar shows `admin (admin)` after the
  pass-through login

### decisions

- tiles and panels are scoped to the selected project - the selector in the
  top bar decides what everything below shows
- "Not Started" is the display label, `todo` stays the stored status value
  (screenshot vs spec - both satisfied)
- priority stays `text` (day 1 decision), values low/normal/high
- `assigned_to` has no UI yet - needs a user picker, which needs day 2/3
  first. the column and service support it

## day 5 - countdowns (16 oct 2026)

### done

- `countdowns` table embedded (project_id nullable for global countdowns,
  title, description, due_time, created_by) + indexes; created on launch
  without touching the existing tables
- `CountdownService`: list (global + the project's, due_time ascending),
  create (project or global), update, remove, upcoming/past counts
- dashboard Countdowns panel: add bar (title, due datetime, Global
  checkbox), live remaining label ticking every second straight from
  `due_time` (never cached), overdue state, edit dialog, delete. global
  countdowns show "(global)" and appear for every project
- Active/Completed Countdowns tiles show real numbers now

### decisions

- the spec's countdowns table has no status column, so "active" means "not
  past due yet" and is computed from due_time - if the Active/Paused badges
  from the screenshot are wanted, that's a schema change (add status), say
  the word and it goes in as a follow-up
- whether a countdown is global is fixed at creation (edit changes
  title/description/due only)

## day 6 - links + files (16 oct 2026)

### done

- `project_links` + `project_files` tables embedded with indexes
- `LinkService`: list/add/update/remove, urls validated in the service
  (http/https only, must parse, host required) - not in the ui
- `FileService`: uploads copy the bytes to disk under the app data folder
  (`<AppData>/Orbit/files/<uuid>`), the database only stores the row
  (filename, storage_key, mime type, size, uploader). 100 MB cap, empty
  files rejected, failed inserts clean the copy up. remove kills the row
  and the stored bytes
- Links panel on the dashboard: title+url add bar (validation errors shown),
  open (clickable title), edit dialog, delete
- Files panel on the dashboard (the separate panel the spec asked for):
  Upload... button, rows with filename, size, open, delete

### decisions

- cascade on project delete for tasks/countdowns/links/files (fk
  `on delete cascade`) - the "what happens when a project is deleted"
  question from day 3 answered as cascade; flag it if you want restrict
  instead
- file type allowlist not enforced, only size + non-empty - can tighten
  when we know what types matter
- upload folder lives under the user's AppData, outside the git repo

### next

day 2 is still pending (real login, argon2id, sessions + settings tables,
logout/expiry wiring, remember-me storage - the qtkeychain vs encrypted
file question is still open), then day 3 (projects crud + permissions).
days 7-9: settings page, caching, audit/admin.

### open questions (days 4-6)

- countdown Active/Paused status column (screenshot showed badges, spec has
  no status) - add or leave computed-from-due_time?
- cascade vs restrict when a project is deleted - I went cascade
- `to-dolist.md` keep or delete? still unanswered from day 1

## day 6-8 plus gui polish (16 oct 2026)

### gui polish

- dialogs (task/countdown/link edit, message boxes, tooltips), inputs and
  scrollbars styled in both themes; rows/buttons/hover states consistent
  across all four dashboard panels
- full light theme (`ThemeLight.qss`) alongside dark; "System" follows the
  windows app theme setting
- dashboard header per the reference screenshot: "Good morning/afternoon/
  evening, <name>" plus a live clock

### day 7 - settings (stored in postgres)

- `settings` table (one row per user: notifications_enabled, theme,
  timezone) - created on launch like the others
- `SettingsService`: read/upsert, plus the signed-in user's row kept in
  memory for timezone formatting (`formatInTimezone`)
- settings page is real now: theme (System/Dark/Light, applies live),
  timezone (UTC-12..UTC+14, used for the absolute "due ..." line on
  countdown rows), notifications toggle, and change-password (checks the
  current password, stores the new one hashed, tries to revoke the user's
  sessions - the revoke needs the day 2 sessions table and silently skips
  until then)
- theme preference loads at startup before the window is built

### day 8 - caching

- `src/cache/Cache`: in-memory ttl cache (30s), prefix invalidation, all
  on the ui thread. never stores passwords/auth/session data
- wired into every read path: projects list, task lists + tile counts,
  countdown lists + counts, links, files. every create/update/delete
  invalidates its prefix immediately, so what you see after a click is
  always fresh
- countdown remaining time is recalculated from the real due_time every
  second - the cache only ever holds the row data, never the countdown

### decisions (days 6-8)

- timezone list is whole-hour offsets only for now (half-hour zones like
  UTC+5:30 aren't in the combo; the parser accepts them)
- "System" theme reads the windows AppsUseLightTheme registry value
- cache keys are project-scoped, not user-scoped - fine while the app is
  single-user-ish (everyone is the bootstrap admin); will need per-user
  keys once day 3 permissions/multi-user land
- password change revocation of sessions is best-effort until day 2's
  sessions table exists

### next

day 2 (real login, argon2id, sessions - remember-me storage question still
open), day 3 (projects crud + permissions), day 9 (audit + admin).

### open questions (days 7-8)

- timezone: ok with whole hours only, or add half-hour zones now?
- notifications toggle is stored but nothing fires it yet (no desktop
  notification code) - wire real notifications later or drop the toggle?
