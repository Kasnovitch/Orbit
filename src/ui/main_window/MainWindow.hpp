#pragma once

#include <QMainWindow>
#include <QVector>

class QLabel;
class ProjectSelector;
class QPushButton;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    void setUser(const QString& username, const QString& role);

signals:
    void logoutRequested();

private:
    void navigate(int index);
    QWidget* buildPlaceholderPage(const QString& heading, const QString& hint);

    QStackedWidget* m_pages = nullptr;
    QVector<QPushButton*> m_navButtons;
    ProjectSelector* m_projectSelector = nullptr;
    QLabel* m_userLabel = nullptr;
    QPushButton* m_logoutButton = nullptr;
};
