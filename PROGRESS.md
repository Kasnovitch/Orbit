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
