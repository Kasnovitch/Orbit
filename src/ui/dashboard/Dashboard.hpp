#pragma once

#include <QWidget>

class CountdownPanel;
class StatTile;
class TaskPanel;

class Dashboard : public QWidget
{
    Q_OBJECT
public:
    explicit Dashboard(qint64 userId, QWidget* parent = nullptr);

    // everything on the dashboard follows the selected project
    void setProject(qint64 projectId);

private:
    void refreshStats();

    qint64 m_userId = 0;
    qint64 m_projectId = 0;
    StatTile* m_activeTasks = nullptr;
    StatTile* m_completedTasks = nullptr;
    StatTile* m_upcomingDeadlines = nullptr;
    StatTile* m_activeCountdowns = nullptr;
    StatTile* m_completedCountdowns = nullptr;
    CountdownPanel* m_countdownPanel = nullptr;
    TaskPanel* m_taskPanel = nullptr;
};
