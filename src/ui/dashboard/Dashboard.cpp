#include "src/ui/dashboard/Dashboard.hpp"

#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>

#include "src/services/tasks/TaskService.hpp"
#include "src/ui/dashboard/countdown_panel/CountdownPanel.hpp"
#include "src/ui/dashboard/files_panel/FilesPanel.hpp"
#include "src/ui/dashboard/links_panel/LinksPanel.hpp"
#include "src/ui/dashboard/task_panel/TaskPanel.hpp"
#include "src/ui/widgets/Widgets.hpp"

Dashboard::Dashboard(qint64 userId, QWidget* parent)
    : QWidget(parent)
    , m_userId(userId)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* tiles = new QHBoxLayout();
    tiles->setSpacing(16);
    m_activeTasks = new StatTile("Active Tasks", this);
    m_completedTasks = new StatTile("Completed Tasks", this);
    m_upcomingDeadlines = new StatTile("Upcoming Deadlines", this);
    m_activeCountdowns = new StatTile("Active Countdowns", this);
    m_completedCountdowns = new StatTile("Completed Countdowns", this);
    tiles->addWidget(m_activeTasks);
    tiles->addWidget(m_completedTasks);
    tiles->addWidget(m_upcomingDeadlines);
    tiles->addWidget(m_activeCountdowns);
    tiles->addWidget(m_completedCountdowns);
    tiles->addStretch();

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* left = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(16);
    m_countdownPanel = new CountdownPanel(left);
    m_taskPanel = new TaskPanel(m_userId, left);
    leftLayout->addWidget(m_countdownPanel, 3);
    leftLayout->addWidget(m_taskPanel, 2);

    auto* right = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(16);
    rightLayout->addWidget(new LinksPanel(right), 1);
    rightLayout->addWidget(new FilesPanel(right), 1);

    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    layout->addLayout(tiles);
    layout->addWidget(splitter, 1);

    connect(m_taskPanel, &TaskPanel::dataChanged, this, &Dashboard::refreshStats);
}

void Dashboard::setProject(qint64 projectId)
{
    if (m_projectId == projectId)
        return;
    m_projectId = projectId;
    m_taskPanel->setProject(projectId);
    refreshStats();
}

void Dashboard::refreshStats()
{
    TaskService tasks;
    m_activeTasks->setValue(QString::number(tasks.countActive(m_projectId)));
    m_completedTasks->setValue(QString::number(tasks.countCompleted(m_projectId)));
    m_upcomingDeadlines->setValue(QString::number(tasks.countDueSoon(m_projectId, 7)));
}
