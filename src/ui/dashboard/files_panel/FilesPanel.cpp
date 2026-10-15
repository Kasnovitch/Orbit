#include "src/ui/dashboard/files_panel/FilesPanel.hpp"

#include <QLabel>
#include <QVBoxLayout>

FilesPanel::FilesPanel(QWidget* parent)
    : Panel("Files", parent)
{
    auto* empty = new QLabel("Files uploaded to the selected project will appear here.", this);
    empty->setObjectName("emptyHint");
    empty->setWordWrap(true);
    m_body->addWidget(empty);
}
