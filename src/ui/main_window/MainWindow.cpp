#include "src/ui/main_window/MainWindow.hpp"

#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "src/ui/dashboard/Dashboard.hpp"
#include "src/ui/dashboard/project_selector/ProjectSelector.hpp"
#include "src/ui/projects/ProjectsScreen.hpp"
#include "src/ui/settings/SettingsScreen.hpp"

MainWindow::MainWindow(const AuthUser& user, QWidget* parent)
    : QMainWindow(parent)
    , m_user(user)
{
    setWindowTitle("Orbit");
    resize(1280, 800);

    auto* central = new QWidget(this);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* sidebar = new QFrame(central);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(200);

    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 24, 16, 16);
    sidebarLayout->setSpacing(8);

    auto* brand = new QLabel("Orbit", sidebar);
    brand->setObjectName("sidebarBrand");
    brand->setWordWrap(true);

    auto* navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);

    const QStringList navLabels = { "Dashboard", "Tasks", "Countdowns",
                                    "Projects", "Teams", "Settings" };
    for (int i = 0; i < navLabels.size(); ++i) {
        auto* button = new QPushButton(navLabels.at(i), sidebar);
        button->setObjectName("navButton");
        button->setCheckable(true);
        navGroup->addButton(button, i);
        m_navButtons.append(button);
        sidebarLayout->addWidget(button);
    }
    sidebarLayout->addStretch();

    m_userLabel = new QLabel(sidebar);
    m_userLabel->setObjectName("sidebarUser");
    m_userLabel->setWordWrap(true);
    m_userLabel->setVisible(false);

    m_logoutButton = new QPushButton("Log out", sidebar);
    m_logoutButton->setObjectName("logoutButton");
    m_logoutButton->setVisible(false);
    connect(m_logoutButton, &QPushButton::clicked, this, &MainWindow::logoutRequested);

    sidebarLayout->addWidget(m_userLabel);
    sidebarLayout->addWidget(m_logoutButton);

    auto* content = new QWidget(central);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 20, 24, 20);
    contentLayout->setSpacing(12);

    auto* topBar = new QHBoxLayout();
    m_projectSelector = new ProjectSelector(content);
    topBar->addStretch();
    topBar->addWidget(m_projectSelector);

    m_pages = new QStackedWidget(content);
    m_dashboard = new Dashboard(m_user.id, m_user.displayName, m_pages);
    m_pages->addWidget(m_dashboard);
    m_pages->addWidget(buildPlaceholderPage(
        "Tasks", "The task list lives on the dashboard for now, tied to the selected project."));
    m_pages->addWidget(buildPlaceholderPage(
        "Countdowns", "Countdowns live on the dashboard panel - nearest deadline first, global ones included."));
    auto* projectsPage = new ProjectsScreen(m_user, m_pages);
    m_pages->addWidget(projectsPage);
    m_pages->addWidget(buildPlaceholderPage(
        "Teams", "Teams stay a placeholder until collaboration is needed."));
    m_pages->addWidget(new SettingsScreen(m_user.id, m_pages));

    contentLayout->addLayout(topBar);
    contentLayout->addWidget(m_pages, 1);

    root->addWidget(sidebar);
    root->addWidget(content, 1);
    setCentralWidget(central);

    // load the projects now that the dashboard is listening
    connect(m_projectSelector, &ProjectSelector::projectChanged,
            m_dashboard, &Dashboard::setProject);
    // the projects page can add/rename/remove - keep the selector in step
    connect(projectsPage, &ProjectsScreen::projectsChanged,
            m_projectSelector, &ProjectSelector::reload);
    m_projectSelector->reload();

    m_navButtons.first()->setChecked(true);
    connect(navGroup, &QButtonGroup::idClicked, this, &MainWindow::navigate);
}

void MainWindow::setUser(const QString& username, const QString& role)
{
    setWindowTitle("Orbit \u2014 " + username);
    m_userLabel->setText(username + " (" + role + ")");
    m_userLabel->setVisible(true);
    m_logoutButton->setVisible(true);
}

void MainWindow::navigate(int index)
{
    m_pages->setCurrentIndex(index);
}

QWidget* MainWindow::buildPlaceholderPage(const QString& heading, const QString& hint)
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);

    auto* headingLabel = new QLabel(heading, page);
    headingLabel->setObjectName("pageHeading");

    auto* placeholder = new QLabel(hint, page);
    placeholder->setObjectName("emptyHint");

    layout->addWidget(headingLabel);
    layout->addWidget(placeholder);
    layout->addStretch();
    return page;
}
