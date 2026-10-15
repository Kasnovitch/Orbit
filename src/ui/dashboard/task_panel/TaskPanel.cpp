#include "src/ui/dashboard/task_panel/TaskPanel.hpp"

#include <QLabel>
#include <QVBoxLayout>

TaskPanel::TaskPanel(QWidget* parent)
    : Panel("Tasks", parent)
{
    auto* empty = new QLabel("Tasks for the selected project will appear here.", this);
    empty->setObjectName("emptyHint");
    empty->setWordWrap(true);
    m_body->addWidget(empty);
}
