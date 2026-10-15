#include "src/app/Application.hpp"

#include <QApplication>
#include <QFile>

#include "src/data/auth/AuthService.hpp"
#include "src/data/auth/SessionWatch.hpp"
#include "src/data/database/Database.hpp"
#include "src/infrastructure/config/AppConfig.hpp"
#include "src/ui/auth/LoginScreen.hpp"
#include "src/ui/main_window/MainWindow.hpp"

int Application::run(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("Asalancountdown");
    app.setOrganizationName("Kasnovitch");

    QFile themeFile(":/styles/Theme.qss");
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text))
        app.setStyleSheet(QString::fromUtf8(themeFile.readAll()));

    // one connection for the whole run of the app
    Database database;
    AppConfig config;

    QString dbStatus;
    bool dbOk = false;
    AuthService auth;
    if (!config.load()) {
        dbStatus = "database: " + config.error();
    } else {
        dbOk = database.open(config.database());
        if (dbOk) {
            database.ensureTables();
            if (auth.seedDefaultUser())
                dbStatus = "first run \u2014 log in with admin / admin123";
        }
        if (dbStatus.isEmpty())
            dbStatus = database.status();
    }

    MainWindow mainWindow;
    LoginScreen login;
    login.setDatabaseStatus(dbStatus);

    AuthUser currentUser;
    SessionWatch sessionWatch(auth);

    auto enterApp = [&]() {
        mainWindow.setUser(currentUser.username, currentUser.role);
        sessionWatch.start(currentUser.sessionId);
        mainWindow.show();
        login.close();
    };

    auto leaveApp = [&](const QString& note) {
        sessionWatch.stop();
        auth.logout(currentUser);
        currentUser = AuthUser();
        mainWindow.hide();
        if (!note.isEmpty())
            login.showNote(note);
        login.clearPassword();
        login.show();
    };

    auto doLogin = [&](const QString& username, const QString& password, bool remember) {
        if (!dbOk) {
            login.showNote("no database connection \u2014 fix that first.");
            return;
        }
        QString error;
        currentUser = auth.login(username, password, remember, &error);
        if (currentUser.id <= 0) {
            login.showNote(error);
            return;
        }
        enterApp();
    };

    QObject::connect(&login, &LoginScreen::loginAttempted, doLogin);

    QObject::connect(&login, &LoginScreen::registerAttempted,
        [&](const QString& username, const QString& password,
            const QString& confirmPassword, bool remember) {
            if (!dbOk) {
                login.showNote("no database connection \u2014 fix that first.");
                return;
            }
            QString error;
            if (!auth.registerUser(username, password, confirmPassword, &error)) {
                login.showNote(error);
                return;
            }
            // account saved, take them straight in
            doLogin(username, password, remember);
        });

    QObject::connect(&mainWindow, &MainWindow::logoutRequested, [&]() {
        leaveApp("");
    });

    QObject::connect(&sessionWatch, &SessionWatch::sessionExpired, [&]() {
        leaveApp("your session ended \u2014 you were away too long, or it hit "
                 "the 7 day limit. log in again.");
    });

    // "remember me" from a previous run?
    if (dbOk)
        currentUser = auth.restoreSession();

    if (currentUser.id > 0)
        enterApp();
    else
        login.show();

    return app.exec();
}
