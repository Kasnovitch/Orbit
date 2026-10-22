#pragma once

#include <QMainWindow>
#include <QVector>

#include "src/data/auth/AuthService.hpp"

class Dashboard;
class QCloseEvent;
class QLabel;
class ProjectSelector;
class QPushButton;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(const AuthUser& user, QWidget* parent = nullptr);

    void setUser(const QString& username, const QString& role);

signals:
    void logoutRequested();

protected:
    // day 10: with "minimize to tray" on, closing hides to the tray
    // instead of quitting
    void closeEvent(QCloseEvent* event) override;

private:
    void navigate(int index);
    void applyWindowFlags();
    QWidget* buildPlaceholderPage(const QString& heading, const QString& hint);

    AuthUser m_user;
    Dashboard* m_dashboard = nullptr;
    QStackedWidget* m_pages = nullptr;
    QVector<QPushButton*> m_navButtons;
    ProjectSelector* m_projectSelector = nullptr;
    QLabel* m_userLabel = nullptr;
    QPushButton* m_logoutButton = nullptr;
};
