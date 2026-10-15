# orbit

desktop project management dashboard. qt 6 (widgets, not qml), cmake, postgresql.
builds and runs on windows.

## building

you need:

- qt 6 (tested with 6.11, mingw kit) - <C:\Qt\6.11.0\mingw_64> on this machine
- a mingw g++ toolchain (we build with <C:\ProgramData\mingw64> - qt's own bundled
  g++ gets eaten by defender on this box, so use any working mingw64 g++ 15)
- cmake 3.21+

configure once, then build:

```
cmake -S . -B build -G "MinGW Makefiles" ^
      -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/mingw_64 ^
      -DCMAKE_CXX_COMPILER=C:/ProgramData/mingw64/mingw64/bin/g++.exe ^
      -DCMAKE_MAKE_PROGRAM=C:/ProgramData/mingw64/mingw64/bin/mingw32-make.exe ^
      -DCMAKE_BUILD_TYPE=Debug

cmake --build build -j 8
```

run `build\Orbit.exe`. `build\OrbitConsole.exe` is a small console shell for
running `request <table>` queries against the same database.

the exe needs the qt dlls next to it (Qt6Core/Gui/Widgets/Sql/Network, libgcc,
libstdc++, libwinpthread), plus `platforms\qwindows.dll` and
`sqldrivers\qsqlpsql.dll`. the build folder on this machine already has them
deployed.

## config

database credentials live in `config/dev/AppConfig.json`, which is gitignored.
first run on a new machine:

```
copy config.example.json config\dev\AppConfig.json
```

then fill in the real host/user/password. never put secrets in source or in a
committed config file.

## the qpsql driver on windows

this is the part that always bites. checklist when you get
"QPSQL driver not loaded":

1. **is the driver even installed?** look for
   `C:\Qt\6.11.0\mingw_64\plugins\sqldrivers\qsqlpsql.dll`. if it's missing,
   open the qt installer and add "postgreSQL SQL driver" under Qt SQL.
2. **libpq.dll** - the driver links against the postgres client library, which
   qt does not ship. put postgres' `bin` on your PATH, or copy `libpq.dll`
   next to the exe (when copying it by hand you'll also need `libintl-9.dll`,
   `libiconv-2.dll`, `libssl-3-x64.dll`, `libcrypto-3-x64.dll`,
   `libwinpthread-1.dll`, `zlib1.dll` - grab them from any postgres windows
   bundle).
3. **local deploy quirk** - if the qt dlls sit next to your exe there is no
   qt.conf, and qt then looks for sql plugins in `<exe dir>\sqldrivers`, not
   in the qt install. copy `qsqlpsql.dll` to `build\sqldrivers\`. miss this
   one and you get "driver not loaded" even though the driver exists.
4. **quick check** - `QSqlDatabase::drivers()` in your code must print
   `QPSQL`. if it lists nothing, it's step 3; if it lists QPSQL but open()
   fails, it's usually step 2 (libpq).

## database

postgresql for dev and prod. the app creates its tables on first run from sql
embedded in `src/data/database/Database.cpp`, and only ever creates tables
that are missing - it never drops or recreates an existing database.

day 1 tables: `users`, `projects`, `tasks`. more tables land on the day their
feature does (see PROGRESS.md).

## project layout

```
src/app         startup / wiring
src/ui          windows, widgets, pages, styles (qss in src/ui/styles)
src/data        database connection, auth/session helpers
src/infrastructure/config   config file loading
src/console     the batch console
src/...         scaffold for later days (services, security, domain, cache, ...)
```

## progress

what's done, what's next, decisions and open questions all live in
[PROGRESS.md](PROGRESS.md).
