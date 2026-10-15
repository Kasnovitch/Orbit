#include "src/ui/dashboard/project_selector/ProjectSelector.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>

ProjectSelector::ProjectSelector(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* label = new QLabel("Project:", this);
    label->setObjectName("selectorLabel");

    m_combo = new QComboBox(this);
    m_combo->setObjectName("projectCombo");
    m_combo->setMinimumWidth(240);
    m_combo->addItem("No projects yet");

    layout->addWidget(label);
    layout->addWidget(m_combo);
    layout->addStretch();

    connect(m_combo, &QComboBox::currentTextChanged, this, &ProjectSelector::projectChanged);
}
