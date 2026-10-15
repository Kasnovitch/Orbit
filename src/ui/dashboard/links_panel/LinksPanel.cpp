#include "src/ui/dashboard/links_panel/LinksPanel.hpp"

#include <QLabel>
#include <QVBoxLayout>

LinksPanel::LinksPanel(QWidget* parent)
    : Panel("Links", parent)
{
    auto* empty = new QLabel("Links saved on the selected project will appear here.", this);
    empty->setObjectName("emptyHint");
    empty->setWordWrap(true);
    m_body->addWidget(empty);
}
