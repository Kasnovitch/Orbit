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
#include "src/ui/settings/SettingsScreen.hpp"

namespace {
constexpr int kDashboardPage = 0;
constexpr int kTeamsPage = 1;
constexpr int kSettingsPage = 2;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Asalancountdown");
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

    auto* brand = new QLabel("Asalancountdown", sidebar);
    brand->setObjectName("sidebarBrand");
    brand->setWordWrap(true);

    auto* navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);

    const QStringList navLabels = { "Dashboard", "Teams", "Settings" };
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
    m_pages->addWidget(new Dashboard(m_pages));
    m_pages->addWidget(buildTeamsPage());
    m_pages->addWidget(new SettingsScreen(m_pages));

    contentLayout->addLayout(topBar);
    contentLayout->addWidget(m_pages, 1);

    root->addWidget(sidebar);
    root->addWidget(content, 1);
    setCentralWidget(central);

    m_navButtons.at(kDashboardPage)->setChecked(true);

    connect(navGroup, &QButtonGroup::idClicked, this, &MainWindow::navigate);
}

void MainWindow::setUser(const QString& username, const QString& role)
{
    setWindowTitle("Asalancountdown \u2014 " + username);
    m_userLabel->setText(username + " (" + role + ")");
    m_userLabel->setVisible(true);
    m_logoutButton->setVisible(true);
}

void MainWindow::navigate(int index)
{
    m_pages->setCurrentIndex(index);
}

QWidget* MainWindow::buildTeamsPage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);

    auto* heading = new QLabel("Teams", page);
    heading->setObjectName("pageHeading");

    auto* placeholder = new QLabel("Coming soon \u2014 teams stay a placeholder until collaboration is needed.", page);
    placeholder->setObjectName("emptyHint");

    layout->addWidget(heading);
    layout->addWidget(placeholder);
    layout->addStretch();
    return page;
}
