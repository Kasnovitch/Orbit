#include "src/ui/projects/ProjectsScreen.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "src/services/permissions/PermissionService.hpp"
#include "src/services/projects/ProjectService.hpp"

namespace {

// small edit dialog, same shape as the task one
class ProjectEditDialog : public QDialog
{
public:
    ProjectEditDialog(const QString& name, const QString& description,
                      QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Edit project");
        auto* form = new QFormLayout(this);

        m_name = new QLineEdit(name, this);
        m_description = new QLineEdit(description, this);

        form->addRow("Name", m_name);
        form->addRow("Description", m_description);

        auto* buttons =
            new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        form->addRow(buttons);
    }

    QString name() const { return m_name->text().trimmed(); }
    QString description() const { return m_description->text().trimmed(); }

private:
    QLineEdit* m_name = nullptr;
    QLineEdit* m_description = nullptr;
};

}

ProjectsScreen::ProjectsScreen(const AuthUser& user, QWidget* parent)
    : QWidget(parent)
    , m_user(user)
{
    auto* layout = new QVBoxLayout(this);
    // padding comes from the main window's content layout - one source,
    // so this page lines up with the dashboard and the top bar
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* heading = new QLabel("Projects", this);
    heading->setObjectName("pageHeading");
    layout->addWidget(heading);

    // add row
    auto* addRow = new QHBoxLayout();
    m_nameInput = new QLineEdit(this);
    m_nameInput->setPlaceholderText("Project name");
    m_descriptionInput = new QLineEdit(this);
    m_descriptionInput->setPlaceholderText("Description (optional)");

    auto* addButton = new QPushButton("Add project", this);
    addButton->setObjectName("primaryButton");
    connect(addButton, &QPushButton::clicked, this, &ProjectsScreen::addProject);
    connect(m_nameInput, &QLineEdit::returnPressed, this, &ProjectsScreen::addProject);

    addRow->addWidget(m_nameInput, 1);
    addRow->addWidget(m_descriptionInput, 2);
    addRow->addWidget(addButton);
    layout->addLayout(addRow);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("emptyHint");
    layout->addWidget(m_statusLabel);

    // the list itself is just stacked frames, no scroll area - projects
    // are expected to stay in the dozens, same as the dashboard panels
    m_listLayout = new QVBoxLayout();
    m_listLayout->setSpacing(8);
    layout->addLayout(m_listLayout);
    layout->addStretch();

    reload();
}

void ProjectsScreen::reload()
{
    ProjectService projects;
    const QVector<ProjectRow> rows = projects.listAll();

    // wipe the old rows
    while (QLayoutItem* item = m_listLayout->takeAt(0)) {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    m_statusLabel->setText(rows.isEmpty()
        ? "no projects yet - add the first one above."
        : QString());
    m_statusLabel->setVisible(rows.isEmpty());

    PermissionService permissions;

    for (const ProjectRow& row : rows) {
        // same row treatment as the dashboard lists - the old frame
        // object name had no style behind it in either theme
        auto* frame = new QFrame(this);
        frame->setObjectName("itemRow");
        auto* line = new QHBoxLayout(frame);
        line->setContentsMargins(16, 12, 16, 12);
        line->setSpacing(12);

        auto* textCol = new QVBoxLayout();
        textCol->setSpacing(2);
        auto* nameLabel = new QLabel(row.name, frame);
        nameLabel->setObjectName("projectName");
        auto* meta = new QLabel(
            (row.description.isEmpty() ? QString() : row.description + "  \u00b7  ")
                + (row.createdByName.isEmpty()
                       ? QString()
                       : "by " + row.createdByName + "  \u00b7  ")
                + (row.status == "archived" ? "archived" : "active"),
            frame);
        meta->setObjectName("rowDetail");
        textCol->addWidget(nameLabel);
        if (!meta->text().isEmpty())
            textCol->addWidget(meta);
        line->addLayout(textCol, 1);

        if (permissions.canModify(m_user, row.id)) {
            auto* editButton = new QPushButton("Edit", frame);
            editButton->setObjectName("rowButton");
            const qint64 id = row.id;
            const QString name = row.name;
            const QString description = row.description;
            connect(editButton, &QPushButton::clicked, this,
                    [this, id, name, description]() {
                        editProject(id, name, description);
                    });
            line->addWidget(editButton);

            auto* archiveButton = new QPushButton(
                row.status == "archived" ? "Restore" : "Archive", frame);
            archiveButton->setObjectName("rowButton");
            const bool archived = row.status == "archived";
            connect(archiveButton, &QPushButton::clicked, this,
                    [this, id, archived]() { toggleArchive(id, archived); });
            line->addWidget(archiveButton);

            auto* deleteButton = new QPushButton("Delete", frame);
            deleteButton->setObjectName("rowButton");
            connect(deleteButton, &QPushButton::clicked, this,
                    [this, id, name]() { removeProject(id, name); });
            line->addWidget(deleteButton);
        }

        m_listLayout->addWidget(frame);
    }
}

void ProjectsScreen::addProject()
{
    ProjectService projects;
    QString error;
    const qint64 id = projects.create(m_nameInput->text(),
                                      m_descriptionInput->text(),
                                      m_user.id, &error);
    if (id == 0) {
        showError(error);
        return;
    }

    m_nameInput->clear();
    m_descriptionInput->clear();
    reload();
    emit projectsChanged();
}

void ProjectsScreen::editProject(qint64 projectId, const QString& name,
                                 const QString& description)
{
    ProjectEditDialog dialog(name, description, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    ProjectService projects;
    QString error;
    if (!projects.update(projectId, dialog.name(), dialog.description(),
                         m_user, &error)) {
        showError(error);
        return;
    }

    reload();
    emit projectsChanged();
}

void ProjectsScreen::toggleArchive(qint64 projectId, bool archived)
{
    ProjectService projects;
    QString error;
    const bool ok = archived
        ? projects.unarchive(projectId, m_user, &error)
        : projects.archive(projectId, m_user, &error);
    if (!ok) {
        showError(error);
        return;
    }

    reload();
    emit projectsChanged();
}

void ProjectsScreen::removeProject(qint64 projectId, const QString& name)
{
    const auto answer = QMessageBox::question(
        this,
        "Delete project",
        "Delete \"" + name + "\"?\n\n"
        "Its tasks, countdowns, links and files go with it.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    ProjectService projects;
    QString error;
    if (!projects.remove(projectId, m_user, &error)) {
        showError(error);
        return;
    }

    reload();
    emit projectsChanged();
}

void ProjectsScreen::showError(const QString& message)
{
    QMessageBox::warning(this, "Projects",
                         message.isEmpty() ? "something went wrong." : message);
}
