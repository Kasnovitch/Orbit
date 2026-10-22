#include "src/ui/dashboard/Dashboard.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

#include "src/services/countdowns/CountdownService.hpp"
#include "src/services/tasks/TaskService.hpp"
#include "src/ui/dashboard/countdown_panel/CountdownPanel.hpp"
#include "src/ui/dashboard/files_panel/FilesPanel.hpp"
#include "src/ui/dashboard/links_panel/LinksPanel.hpp"
#include "src/ui/dashboard/task_panel/TaskPanel.hpp"
#include "src/ui/widgets/Widgets.hpp"

Dashboard::Dashboard(qint64 userId, const QString& displayName, QWidget* parent)
    : QWidget(parent)
    , m_userId(userId)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    // greeting + live clock, per the reference screenshot
    const int hour = QTime::currentTime().hour();
    const QString partOfDay = hour < 12 ? QStringLiteral("morning")
        : hour < 18                             ? QStringLiteral("afternoon")
                                                : QStringLiteral("evening");
    const QString name = displayName.isEmpty() ? QStringLiteral("there") : displayName;

    auto* header = new QHBoxLayout();
    auto* greeting = new QLabel(
        QStringLiteral("Good %1, %2").arg(partOfDay, name), this);
    greeting->setObjectName("dashboardGreeting");

    m_clockLabel = new QLabel(this);
    m_clockLabel->setObjectName("dashboardClock");

    header->addWidget(greeting);
    header->addStretch();
    header->addWidget(m_clockLabel);

    auto* clockTimer = new QTimer(this);
    clockTimer->setInterval(1000);
    connect(clockTimer, &QTimer::timeout, this, &Dashboard::updateClock);
    clockTimer->start();
    updateClock();

    layout->addLayout(header);

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
    m_countdownPanel = new CountdownPanel(m_userId, left);
    m_taskPanel = new TaskPanel(m_userId, left);
    leftLayout->addWidget(m_countdownPanel, 3);
    leftLayout->addWidget(m_taskPanel, 2);

    auto* right = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(16);
    m_linksPanel = new LinksPanel(m_userId, right);
    m_filesPanel = new FilesPanel(m_userId, right);
    rightLayout->addWidget(m_linksPanel, 1);
    rightLayout->addWidget(m_filesPanel, 1);

    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    layout->addLayout(tiles);
    layout->addWidget(splitter, 1);

    connect(m_taskPanel, &TaskPanel::dataChanged, this, &Dashboard::refreshStats);
    connect(m_countdownPanel, &CountdownPanel::dataChanged, this, &Dashboard::refreshStats);
}

void Dashboard::setProject(qint64 projectId)
{
    if (m_projectId == projectId)
        return;
    m_projectId = projectId;
    m_taskPanel->setProject(projectId);
    m_countdownPanel->setProject(projectId);
    m_linksPanel->setProject(projectId);
    m_filesPanel->setProject(projectId);
    refreshStats();
}

void Dashboard::refreshStats()
{
    TaskService tasks;
    m_activeTasks->setValue(QString::number(tasks.countActive(m_projectId)));
    m_completedTasks->setValue(QString::number(tasks.countCompleted(m_projectId)));
    m_upcomingDeadlines->setValue(QString::number(tasks.countDueSoon(m_projectId, 7)));

    CountdownService countdowns;
    m_activeCountdowns->setValue(QString::number(countdowns.countUpcoming(m_projectId)));
    m_completedCountdowns->setValue(QString::number(countdowns.countPast(m_projectId)));
}

void Dashboard::updateClock()
{
    m_clockLabel->setText(
        QTime::currentTime().toString(QStringLiteral("ddd d MMM  \u00b7  HH:mm:ss")));
}
