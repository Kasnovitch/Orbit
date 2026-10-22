#include "src/app/Application.hpp"

#include <QApplication>
#include <QSqlQuery>

#include "src/data/auth/AuthService.hpp"
#include "src/data/database/Database.hpp"
#include "src/infrastructure/config/AppConfig.hpp"
#include "src/services/projects/ProjectService.hpp"
#include "src/services/settings/SettingsService.hpp"
#include "src/ui/auth/LoginScreen.hpp"
#include "src/ui/main_window/MainWindow.hpp"
#include "src/ui/styles/Theme.hpp"

namespace {

// temporary identity bootstrap until day 2 (real login) and day 3
// (projects crud) land: seed the default admin, treat the first active
// user as who "we" are, and make sure one project exists to work in
AuthUser bootstrapIdentity()
{
    AuthUser user;

    AuthService auth;
    auth.seedDefaultUser();

    QSqlQuery find;
    if (!find.exec("select id, username, display_name, role from users "
                   "where is_active = true order by id limit 1")
        || !find.next())
        return user;

    user.id = find.value(0).toLongLong();
    user.username = find.value(1).toString();
    user.displayName = find.value(2).toString();
    user.role = find.value(3).toString();

    ProjectService projects;
    projects.ensureGeneralProject(user.id);

    return user;
}

}

int Application::run(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("Orbit");
    app.setOrganizationName("Kasnovitch");

    OrbitTheme::apply("dark");

    // one connection for the whole run of the app
    Database database;
    AppConfig config;

    QString dbStatus;
    if (!config.load()) {
        dbStatus = "database: " + config.error();
    } else if (database.open(config.database())) {
        database.ensureTables();
        dbStatus = database.status();
    } else {
        dbStatus = database.status();
    }

    // no-op if the db is down - user.id stays 0 and the panels stay empty
    const AuthUser user = bootstrapIdentity();

    // day 7: the saved theme/timezone etc, straight from the settings table
    SettingsService::loadCurrent(user.id);
    OrbitTheme::apply(SettingsService::currentSettings().theme);

    MainWindow mainWindow(user);
    LoginScreen login;
    login.setDatabaseStatus(dbStatus);

    // day 1: any input gets you in, real login lands on day 2
    QObject::connect(&login, &LoginScreen::loginAttempted, [&]() {
        mainWindow.setUser(user.username, user.role);
        mainWindow.show();
        login.close();
    });

    login.show();
    return app.exec();
}
