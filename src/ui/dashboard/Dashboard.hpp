#pragma once

#include <QWidget>

class QLabel;
class CountdownPanel;
class FilesPanel;
class LinksPanel;
class StatTile;
class TaskPanel;

class Dashboard : public QWidget
{
    Q_OBJECT
public:
    explicit Dashboard(qint64 userId, const QString& displayName,
                       QWidget* parent = nullptr);

    // everything on the dashboard follows the selected project
    void setProject(qint64 projectId);

private:
    void refreshStats();
    void updateClock();

    qint64 m_userId = 0;
    qint64 m_projectId = 0;
    QLabel* m_clockLabel = nullptr;
    StatTile* m_activeTasks = nullptr;
    StatTile* m_completedTasks = nullptr;
    StatTile* m_upcomingDeadlines = nullptr;
    StatTile* m_activeCountdowns = nullptr;
    StatTile* m_completedCountdowns = nullptr;
    CountdownPanel* m_countdownPanel = nullptr;
    TaskPanel* m_taskPanel = nullptr;
    LinksPanel* m_linksPanel = nullptr;
    FilesPanel* m_filesPanel = nullptr;
};
