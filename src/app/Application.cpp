#include "src/app/Application.hpp"

#include <QApplication>
#include <QFile>

#include "src/data/database/Database.hpp"
#include "src/infrastructure/config/AppConfig.hpp"
#include "src/ui/auth/LoginScreen.hpp"
#include "src/ui/main_window/MainWindow.hpp"

int Application::run(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("Orbit");
    app.setOrganizationName("Kasnovitch");

    QFile themeFile(":/styles/Theme.qss");
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text))
        app.setStyleSheet(QString::fromUtf8(themeFile.readAll()));

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

    MainWindow mainWindow;
    LoginScreen login;
    login.setDatabaseStatus(dbStatus);

    // day 1: any input gets you in, real login lands on day 2
    QObject::connect(&login, &LoginScreen::loginAttempted, [&]() {
        mainWindow.show();
        login.close();
    });

    login.show();
    return app.exec();
}
