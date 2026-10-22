#include "src/ui/dashboard/project_selector/ProjectSelector.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>

#include "src/services/projects/ProjectService.hpp"

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

    layout->addWidget(label);
    layout->addWidget(m_combo);
    layout->addStretch();

    connect(m_combo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0)
            emit projectChanged(m_combo->itemData(index).toLongLong());
    });
}

void ProjectSelector::reload()
{
    const qint64 previous = currentProjectId();

    ProjectService service;
    const QVector<ProjectRef> projects = service.listActive();

    m_combo->clear();
    for (const ProjectRef& project : projects)
        m_combo->addItem(project.name, project.id);

    if (projects.isEmpty())
        m_combo->addItem("No projects yet", 0);

    // keep the same project selected across reloads
    for (int i = 0; i < m_combo->count(); ++i) {
        if (m_combo->itemData(i).toLongLong() == previous) {
            m_combo->setCurrentIndex(i);
            return;
        }
    }
    if (m_combo->count() > 0)
        m_combo->setCurrentIndex(0);
}

qint64 ProjectSelector::currentProjectId() const
{
    return m_combo->currentData().toLongLong();
}
