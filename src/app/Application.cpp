#include "src/app/Application.hpp"

#include <QApplication>

#include "src/data/auth/AuthService.hpp"
#include "src/data/auth/SessionWatch.hpp"
#include "src/data/database/Database.hpp"
#include "src/infrastructure/config/AppConfig.hpp"
#include "src/services/notifications/NotificationService.hpp"
#include "src/services/projects/ProjectService.hpp"
#include "src/services/settings/SettingsService.hpp"
#include "src/services/share/ShareService.hpp"
#include "src/ui/auth/LoginScreen.hpp"
#include "src/ui/main_window/MainWindow.hpp"
#include "src/ui/styles/Theme.hpp"

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

    AuthService auth;
    auth.seedDefaultUser();

    SessionWatch watch(auth);
    auto& notifications = NotificationService::instance();

    LoginScreen login;
    login.setDatabaseStatus(dbStatus);

    // one window per login; rebuilt on each login so the dashboard
    // always picks up the signed-in user's id
    MainWindow* mainWindow = nullptr;
    AuthUser currentUser;

    auto leaveApp = [&](const QString& note) {
        watch.stop();
        notifications.stop();
        ShareService::instance().stop();
        if (currentUser.sessionId > 0)
            auth.logout(currentUser);
        currentUser = AuthUser();
        // deleteLater: the logout signal is emitted from mainWindow itself
        if (mainWindow)
            mainWindow->deleteLater();
        mainWindow = nullptr;
        OrbitTheme::apply("dark");
        login.show();
        login.clearPassword();
        login.showNote(note);
    };

    // tray menu "Quit" leaves the app entirely (vs logout -> login screen)
    QObject::connect(&notifications, &NotificationService::quitRequested,
                     &app, [&]() {
        leaveApp("quit from the tray.");
        app.quit();
    });

    auto enterApp = [&](const AuthUser& user) {
        currentUser = user;

        ProjectService projects;
        projects.ensureGeneralProject(user.id);

        // day 7: the saved theme/timezone etc, straight from the settings table
        SettingsService::loadCurrent(user.id);
        OrbitTheme::apply(SettingsService::currentSettings().theme);

        mainWindow = new MainWindow(user);
        mainWindow->setUser(user.username, user.role);
        mainWindow->show();
        login.close();
        watch.start(user.sessionId);

        // tray + desktop notifications (day 10); the tray's restore
        // action brings the window back from minimize-to-tray
        notifications.attachWindow(mainWindow);
        notifications.start(user.id);

        // day 12: local share - start the phone-upload server if the
        // user turned it on in settings
        if (SettingsService::currentSettings().shareEnabled)
            ShareService::instance().start(user.id);

        // logged out from the sidebar button
        QObject::connect(mainWindow, &MainWindow::logoutRequested, &watch, [&]() {
            leaveApp("logged out.");
        });
    };

    QObject::connect(&watch, &SessionWatch::sessionExpired, &watch, [&]() {
        leaveApp("session expired \u2014 log in again.");
    });

    QObject::connect(&login, &LoginScreen::loginAttempted,
                     [&](const QString& username, const QString& password,
                         bool remember) {
        QString error;
        const AuthUser user = auth.login(username, password, remember, &error);
        if (user.id == 0) {
            login.showNote(error);
            login.clearPassword();
            return;
        }
        enterApp(user);
    });

    QObject::connect(&login, &LoginScreen::registerAttempted,
                     [&](const QString& username, const QString& password,
                         const QString& confirmPassword, bool remember) {
        Q_UNUSED(remember)
        QString error;
        if (!auth.registerUser(username, password, confirmPassword, &error)) {
            login.showNote(error);
            login.clearPassword();
            return;
        }
        login.showNote("account created \u2014 log in with it.");
        login.clearPassword();
    });

    // coming back from a previous run with "remember me" ticked
    const AuthUser restored = auth.restoreSession();
    if (restored.id > 0)
        enterApp(restored);
    else
        login.show();

    return app.exec();
}
