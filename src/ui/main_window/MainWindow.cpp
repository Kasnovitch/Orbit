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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
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
    m_pages->addWidget(new Dashboard(m_pages));
    m_pages->addWidget(buildPlaceholderPage(
        "Tasks", "Tasks arrive on day 4 \u2014 list, filters and real counts."));
    m_pages->addWidget(buildPlaceholderPage(
        "Countdowns", "Countdowns arrive on day 5 \u2014 live timers and progress bars."));
    m_pages->addWidget(buildPlaceholderPage(
        "Projects", "Projects arrive on day 3 \u2014 crud plus the project selector."));
    m_pages->addWidget(buildPlaceholderPage(
        "Teams", "Teams stay a placeholder until collaboration is needed."));
    m_pages->addWidget(new SettingsScreen(m_pages));

    contentLayout->addLayout(topBar);
    contentLayout->addWidget(m_pages, 1);

    root->addWidget(sidebar);
    root->addWidget(content, 1);
    setCentralWidget(central);

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
