#include "src/ui/main_window/MainWindow.hpp"

#include <QButtonGroup>
#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "src/services/notifications/NotificationService.hpp"
#include "src/services/settings/SettingsService.hpp"
#include "src/ui/dashboard/Dashboard.hpp"
#include "src/ui/dashboard/project_selector/ProjectSelector.hpp"
#include "src/ui/projects/ProjectsScreen.hpp"
#include "src/ui/settings/SettingsScreen.hpp"

namespace {

// one fixed height for every sidebar button so the nav column reads as
// a single aligned rail instead of a ragged list
constexpr int navButtonHeight = 38;

}

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

    // ---- sidebar ----------------------------------------------------
    auto* sidebar = new QFrame(central);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(220);

    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 20, 16, 16);
    sidebarLayout->setSpacing(4);

    auto* brand = new QLabel("Orbit", sidebar);
    brand->setObjectName("sidebarBrand");
    sidebarLayout->addWidget(brand);
    sidebarLayout->addSpacing(12);

    auto* navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);

    const QStringList navLabels = { "Dashboard", "Tasks", "Countdowns",
                                    "Projects", "Teams", "Settings" };
    for (int i = 0; i < navLabels.size(); ++i) {
        auto* button = new QPushButton(navLabels.at(i), sidebar);
        button->setObjectName("navButton");
        button->setCheckable(true);
        button->setFixedHeight(navButtonHeight);
        navGroup->addButton(button, i);
        m_navButtons.append(button);
        sidebarLayout->addWidget(button);
    }
    sidebarLayout->addStretch();

    // footer: separator, then the signed-in user and the logout button,
    // all on the same left edge as the nav buttons above them
    auto* separator = new QFrame(sidebar);
    separator->setObjectName("sidebarSeparator");
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(1);
    sidebarLayout->addWidget(separator);
    sidebarLayout->addSpacing(12);

    m_userLabel = new QLabel(sidebar);
    m_userLabel->setObjectName("sidebarUser");
    m_userLabel->setWordWrap(true);
    m_userLabel->setVisible(false);

    m_logoutButton = new QPushButton("Log out", sidebar);
    m_logoutButton->setObjectName("logoutButton");
    m_logoutButton->setFixedHeight(navButtonHeight);
    m_logoutButton->setVisible(false);
    connect(m_logoutButton, &QPushButton::clicked, this, &MainWindow::logoutRequested);

    sidebarLayout->addWidget(m_userLabel);
    sidebarLayout->addSpacing(6);
    sidebarLayout->addWidget(m_logoutButton);

    // ---- content ----------------------------------------------------
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

    auto* settingsPage = new SettingsScreen(m_user, m_pages);
    m_pages->addWidget(settingsPage);

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
    // settings can flip always-on-top - re-apply without restarting
    connect(settingsPage, &SettingsScreen::settingsChanged,
            this, &MainWindow::applyWindowFlags);
    m_projectSelector->reload();

    applyWindowFlags();

    m_navButtons.first()->setChecked(true);
    connect(navGroup, &QButtonGroup::idClicked, this, &MainWindow::navigate);
}

void MainWindow::setUser(const QString& username, const QString& role)
{
    setWindowTitle("Orbit \u2014 " + username);
    m_userLabel->setText(username + "  \u00b7  " + role);
    m_userLabel->setVisible(true);
    m_logoutButton->setVisible(true);
}

void MainWindow::navigate(int index)
{
    m_pages->setCurrentIndex(index);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (SettingsService::currentSettings().minimizeToTray
        && NotificationService::instance().canHideToTray()) {
        hide();
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::applyWindowFlags()
{
    const bool onTop = SettingsService::currentSettings().alwaysOnTop;
    if (windowFlags().testFlag(Qt::WindowStaysOnTopHint) == onTop)
        return;

    const bool wasVisible = isVisible();
    setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
    // changing flags hides the window - bring it back if it was up
    if (wasVisible)
        show();
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
