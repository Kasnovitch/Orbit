#include "src/ui/dashboard/countdown_panel/CountdownPanel.hpp"

#include <QLabel>
#include <QVBoxLayout>

CountdownPanel::CountdownPanel(QWidget* parent)
    : Panel("Countdowns", parent)
{
    auto* empty = new QLabel("Nearest deadlines will show up here first.", this);
    empty->setObjectName("emptyHint");
    empty->setWordWrap(true);
    m_body->addWidget(empty);
}
