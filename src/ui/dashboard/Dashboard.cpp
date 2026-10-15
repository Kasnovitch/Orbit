#include "src/ui/dashboard/Dashboard.hpp"

#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>

#include "src/ui/dashboard/countdown_panel/CountdownPanel.hpp"
#include "src/ui/dashboard/files_panel/FilesPanel.hpp"
#include "src/ui/dashboard/links_panel/LinksPanel.hpp"
#include "src/ui/dashboard/task_panel/TaskPanel.hpp"
#include "src/ui/widgets/Widgets.hpp"

Dashboard::Dashboard(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* tiles = new QHBoxLayout();
    tiles->setSpacing(16);
    tiles->addWidget(new StatTile("Active Tasks", this));
    tiles->addWidget(new StatTile("Completed Tasks", this));
    tiles->addWidget(new StatTile("Upcoming Deadlines", this));
    tiles->addWidget(new StatTile("Active Countdowns", this));
    tiles->addWidget(new StatTile("Completed Countdowns", this));
    tiles->addStretch();

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* left = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(16);
    leftLayout->addWidget(new CountdownPanel(left), 3);
    leftLayout->addWidget(new TaskPanel(left), 2);

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
}
