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

## day 2 - real login (22 oct 2026)

### done

- libsodium 1.0.22 vendored under `third_party/libsodium/` (official
  mingw static build + a small compat shim for `memset_explicit`, which
  older mingw-w64 CRTs lack). linked into the Orbit target only, not the
  console
- passwords: argon2id via `crypto_pwhash_str` (interactive params: ops 2,
  64 MB). the old pbkdf2 rows still verify (dual-verify on the `$argon2`
  prefix) and upgrade to argon2 the next time the password is changed
- `sessions` table embedded (token hash only, never the raw token; 30 min
  idle + 7 day absolute expiry, revoked_at, unique token_hash + indexes)
- `login()` inserts the session row, updates `last_login_at`, cleans up
  expired sessions on the way through. `restoreSession()` re-reads the
  saved token, validates it against the table, slides the idle window
- remember-me: encrypted local file (libsodium secretbox), not QtKeychain
  - the question was dodged three times and "do it all" was the answer,
  so the simplest self-contained option went in (see decisions)
- `SessionWatch` is live: 30 min idle or 7 day absolute, whichever comes
  first, then back to the login screen with "session expired"
- logout button wired; logout revokes the session row and wipes the
  remember file; next login starts clean
- register mode on the login screen is real: creates a `user`-role
  account, argon2id-hashed
- day 1's pass-through login and the bootstrap identity are gone - you
  either restore a saved session or log in for real. the temporary
  bootstrap admin seeding stays (`seedDefaultUser`) so a fresh database
  still comes up with admin/admin123

### decisions

- remember-me = encrypted file at `<AppData>/Orbit/remember.bin`, key
  derived from username + windows MachineGuid via argon2id. protects
  against the file being copied to another machine, not against someone
  already on this one. QtKeychain question closed as "encrypted file"
- argon2id interactive parameters - noticeable but not annoying at the
  login prompt; bump to moderate if the machine is fast enough
- pbkdf2 kept only as a verify fallback for the seeded admin row

## day 3 - projects crud + permissions (22 oct 2026)

### done

- `permissions` table embedded (user_id + project_id unique, can_write).
  owner (`created_by`) and admins never need a row - they bypass the table
- `PermissionService`: canModify (admin > owner > write grant), grant/revoke
  ready for a future share dialog
- `ProjectService` CRUD: create/update/archive/unarchive/delete, every
  mutation permission-checked in the service (never the UI). delete
  cascades (confirmed in the prompt before it runs)
- Projects page (sidebar item 3) is real now: add row, project list with
  owner + status, Edit / Archive-Restore / Delete buttons that hide when
  you can't touch the project. any change refreshes the top-bar selector
- the day 4-8 bootstrap comment about projects being temporary is done -
  `ensureGeneralProject` now just makes sure the selector never starts
  empty

### decisions

- permission model: admin/owner bypass + explicit write grants. read is
  implicit for everyone who can see the project (visibility column exists
  but is unused; sharing UI is a later thing)
- cache keys are still project-scoped, not user-scoped - fine while one
  machine is one user; revisit when sharing lands

## day 9 - audit + admin (22 oct 2026)

### done

- `audit_logs` table embedded (user_id nullable, action, entity_type,
  entity_id, metadata text, created_at + indexes)
- `AuditService`: fire-and-forget insert (audit never breaks the action
  it records), `recent(limit)` for the admin view. no secrets ever go
  into metadata
- wired into: project create/update/archive/unarchive/delete, permission
  grant/revoke, account register, password change, enable/disable, role
  change
- Settings page, admin only: Users panel (username, role combo, active
  checkbox, last login) - enable/disable kicks the target's sessions,
  you can't disable yourself (box is greyed out); Recent activity panel
  showing the last 20 audit entries
- `AuthService` gained listUsers / setUserActive / setUserRole

### decisions

- audit metadata is plain text, not jsonb - the entries are single facts
  (project name, role name, read/write), jsonb can wait
- disabling a user revokes their sessions, same rule as a password change
- the users list and audit log are read-only refresh-on-change; no
  pagination yet (limit 20 for the log, all users in the list)

### open questions (days 2-9)

- countdown Active/Paused status column (screenshot showed badges, spec
  has no status) - add or leave computed-from-due_time?
- cascade vs restrict when a project is deleted - I went cascade
- `to-dolist.md` keep or delete? still unanswered from day 1
- timezone: whole hours only, or half-hour zones now?
- notifications toggle stored but nothing fires it - wire later or drop?
  (answered day 10: wired)

## day 10 - menu polish + settings mvp (22 oct 2026)

### done

- sidebar rebuilt as a single aligned rail: consistent 38px nav buttons
  on one left edge, brand/nav/footer in fixed sections, separator line
  above the signed-in user block, logout button the same height as the
  nav buttons
- settings page rebuilt on one shared form shape (same label column,
  field width, row spacing) so every panel lines up down the page;
  panels: Appearance, Notifications, Window, Account (+ admin sections)
- notifications are real now: `NotificationService` owns a system tray
  icon (teal O, drawn at runtime) with Open/Quit menu. a 60s timer
  pops balloons for countdowns due within the lead time and tasks
  due/overdue; each item notifies once per due time. the old "toggle
  stored but nothing fires" gap is closed
- window settings: "always on top" (pin above other windows, re-applied
  live without restart) and "minimize to tray" (closing hides to the
  tray, restore from the icon or tray menu; Quit there exits the app)
- `settings` table gained `always_on_top`, `minimize_to_tray`,
  `notify_minutes_before` - additive `ensureColumn` migration (checks
  information_schema, alter table add column), fresh installs get them
  in the day 7 ddl, existing databases upgrade in place
- settings saves are audited (`account.settings_change`) like the other
  account changes

### decisions

- tray icon appears only while logged in and only when notifications or
  minimize-to-tray is on - otherwise Orbit stays out of the tray
- reminder lead time is a spinbox 1-240 minutes, default 15
- notifications dedupe per run (kind:id:due_time key, capped at 400
  keys) - restart the app to re-arm reminders
- minimize-to-tray applies on close only when the tray is actually
  visible; if the tray can't show, close quits as before
- always-on-top uses the native window flag; toggling it re-shows the
  window because changing flags hides it on windows

## day 11 - spec leftovers + ui alignment (23 oct 2026)

### done

- `teams` and `project_members` tables created (day 11 schema group +
  indexes on created_by/project_id/user_id) - table only per the spec,
  the feature stays off until collaboration is actually needed
- task assignment: `TaskRow`/`TaskService` now carry `assigned_to` +
  assignee display name (list joins users), create/update write it, the
  task edit dialog got an "Assign to" combo (active users, disabled
  ones marked, current assignee kept even if disabled), task rows show
  a detail line under the title: priority, assignee, due time
- transactions on multi-statement account changes (spec: use
  transactions when changing related records): login (session insert +
  last_login_at), change password (hash update + session revoke),
  enable/disable (flag update + session revoke). small RAII
  `DbTransaction` in AuthService.cpp - rolls back on early return
- ui alignment pass:
  - removed the double page padding - every page had its own 24px
    margins on top of the main window's 24/20px content margins, so
    headings sat 48px in and out of line with the project selector;
    pages are now 0-margin and the content layout is the single source
    of padding
  - stat tiles share the row evenly (equal stretch each) instead of
    hugging their labels with a ragged gap
  - project rows: `panelFrame` object name had no style in either
    theme - now `itemRow` like every other list, name on a new
    `projectName` style, meta on `rowDetail`, action buttons use
    `rowButton` instead of unstyled flat buttons
  - links and files lists moved into scroll areas like tasks and
    countdowns, so long lists scroll instead of overflowing
- build dir was reconfigured to a Visual Studio generator by an editor
  at some point (17:06 22 oct), clobbering the MinGW cache - restored
  with the README configure command (cache + CMakeFiles wiped, no
  source impact)

### decisions

- assignee picker only lists active users; a task assigned to a now
  disabled user still shows it (marked disabled in the combo)
- unassigned is stored as null (`assigned_to` nullable per spec),
  shown as no detail line rather than "unassigned"
- transactions cover the account flows where two statements must land
  together; single-statement mutations (projects, tasks, links, files,
  permissions) stay plain - postgres already makes those atomic
- teams page keeps its placeholder text - tables exist, feature off

## day 12 - countdown pause, local share, to-dolist closed out

### what we did

- countdowns can be paused now (the last spec leftover):
  - `status` column on countdowns (`active`/`paused`, default active) -
    fresh installs get it in the day 5 DDL, existing databases through
    the usual `ensureColumn` migration
  - `CountdownService::setPaused(id, paused)` flips it and invalidates
    the countdowns cache; the list query now selects status
  - the "Active Countdowns" dashboard tile only counts live ones -
    paused countdowns drop out of `countUpcoming`
  - paused countdowns don't nag: the notification query skips them too
    (`and status <> 'paused'`) - they stay on the list but stay quiet
  - each row gets a Pause/Resume button (same `rowButton` style as
    Edit); paused rows show "paused" in the detail style instead of a
    ticking remaining time, and the 1s tick skips them
- local share (phone -> pc), the last to-dolist item:
  - `ShareService` - a tiny http server on `QTcpServer` (already
    linked, no new Qt module). binds 8787-8789, mints a fresh random
    token per start, and only serves `/t/<token>/...` - anything else
    is a 404 so the server doesn't advertise itself on the network
  - the page is one form (project dropdown + file input), no
    javascript - works from any phone browser on the same wi-fi.
    upload is a plain multipart POST; the body is staged under a temp
    dir with the original filename and pushed through the same
    `FileService::upload` as a desktop upload (same 100 MB cap, same
    mime/size validation, same cache invalidation), attributed to the
    signed-in user
  - after an upload it 303-redirects back with a banner (uploaded
    file name, or the error) and the project dropdown stays where the
    phone left it
  - a successful upload emits `fileReceived(projectId)` - the
    dashboard's files panel reloads if that project is on screen,
    no polling
  - settings got a "Sharing" panel: one checkbox + the link (selectable
    text). turn it on, the server starts immediately and the link
    appears; turn it off, it stops. the setting persists
    (`settings.share_enabled`, fresh DDL + ensureColumn) and the
    server starts at login if it was left on, stops at logout
- `to-dolist.md` is closed out - every box ticked, header note added.
  kept as the original spec/checklist reference. the one open question
  (half-hour timezones) is called out as out of scope; everything else
  from the list shipped
- notification service: countdown reminder query now skips paused
  countdowns so a paused deadline doesn't keep firing

### decisions

- share server is upload-only - phone pushes a file, nothing to
  download back. simplest thing that closes the "phone to pc" item
- token in the url, not a login page - anyone with the link can upload
  while the server runs (which is only while the pc user is logged in
  and the setting is on). noted in the settings hint text. a full
  phone-side login was overkill for a local convenience feature
- no Qt HttpServer module (not installed with our Qt build) - plain
  QTcpServer + a minimal hand-rolled parser for exactly the two
  requests we need (GET page, POST multipart). one file, no new
  dependencies
- the 100 MB upload cap is FileService's existing one - the share
  path reuses FileService::upload wholesale so there's one cap, one
  validator, one storage layout
- paused countdowns are excluded from the active tile and from
  notifications, but stay visible in the list with a "paused" label -
  they're not deleted or hidden, just quiet
- to-dolist.md is kept (ticked, with a done-as-of-day-12 note) rather
  than deleted - it doubles as the spec reference
