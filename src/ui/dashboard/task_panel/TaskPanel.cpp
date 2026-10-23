#include "src/ui/dashboard/task_panel/TaskPanel.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>

#include "src/data/auth/AuthService.hpp"
#include "src/services/settings/SettingsService.hpp"
#include "src/services/tasks/TaskService.hpp"

namespace {

// display labels per the screenshot; the stored status stays the spec value
QString statusLabel(const QString& status)
{
    if (status == "in_progress")
        return QStringLiteral("In Progress");
    if (status == "completed")
        return QStringLiteral("Completed");
    return QStringLiteral("Not Started");
}

// everything under the task title: priority, assignee, due time - the
// row detail line, built from whatever is set
QString taskDetail(const TaskRow& task)
{
    QStringList parts;

    if (!task.priority.isEmpty() && task.priority != "normal")
        parts << task.priority;
    if (task.assignedTo > 0)
        parts << QStringLiteral("for %1").arg(task.assignedToName);
    if (task.dueTime.isValid())
        parts << QStringLiteral("due %1").arg(SettingsService::formatInTimezone(
            task.dueTime, QStringLiteral("d MMM, HH:mm")));

    return parts.join(QStringLiteral("  \u00b7  "));
}

// small edit dialog for title/contents/priority/due/assignee - file
// local, nothing else in the app needs to know about it
class TaskEditDialog : public QDialog
{
public:
    explicit TaskEditDialog(const TaskRow& task, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Edit task");
        auto* form = new QFormLayout(this);

        m_title = new QLineEdit(task.title, this);
        m_contents = new QPlainTextEdit(task.contents, this);
        m_contents->setMinimumHeight(90);

        m_priority = new QComboBox(this);
        m_priority->addItems({ "low", "normal", "high" });
        m_priority->setCurrentText(task.priority.isEmpty() ? "normal" : task.priority);

        m_assignee = new QComboBox(this);
        m_assignee->addItem("Unassigned", 0);
        AuthService auth;
        const QVector<UserRow> users = auth.listUsers();
        for (const UserRow& user : users) {
            if (!user.active && user.id != task.assignedTo)
                continue;
            const QString label = user.displayName.isEmpty()
                ? user.username
                : QStringLiteral("%1 (%2)").arg(user.displayName, user.username);
            m_assignee->addItem(
                user.active ? label : label + QStringLiteral(" \u2014 disabled"),
                user.id);
        }
        const int assigneeIndex = m_assignee->findData(task.assignedTo);
        m_assignee->setCurrentIndex(assigneeIndex >= 0 ? assigneeIndex : 0);

        m_noDue = new QCheckBox("No due date", this);
        m_due = new QDateTimeEdit(task.dueTime.isValid()
                                      ? task.dueTime
                                      : QDateTime::currentDateTime().addDays(1),
                                  this);
        m_due->setCalendarPopup(true);
        m_noDue->setChecked(!task.dueTime.isValid());
        m_due->setEnabled(task.dueTime.isValid());
        connect(m_noDue, &QCheckBox::toggled, m_due, &QWidget::setDisabled);

        form->addRow("Title", m_title);
        form->addRow("Contents", m_contents);
        form->addRow("Priority", m_priority);
        form->addRow("Assign to", m_assignee);
        form->addRow(QString(), m_noDue);
        form->addRow("Due", m_due);

        auto* buttons =
            new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        form->addRow(buttons);
    }

    QString title() const { return m_title->text().trimmed(); }
    QString contents() const { return m_contents->toPlainText(); }
    QString priority() const { return m_priority->currentText(); }
    qint64 assignedTo() const { return m_assignee->currentData().toLongLong(); }
    QDateTime dueTime() const { return m_noDue->isChecked() ? QDateTime() : m_due->dateTime(); }

private:
    QLineEdit* m_title = nullptr;
    QPlainTextEdit* m_contents = nullptr;
    QComboBox* m_priority = nullptr;
    QComboBox* m_assignee = nullptr;
    QCheckBox* m_noDue = nullptr;
    QDateTimeEdit* m_due = nullptr;
};

}

TaskPanel::TaskPanel(qint64 userId, QWidget* parent)
    : Panel("Tasks", parent)
    , m_userId(userId)
{
    auto* addBar = new QHBoxLayout();
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setObjectName("panelInput");
    m_titleEdit->setPlaceholderText("New task title\u2026");

    auto* addButton = new QPushButton("Add", this);
    addButton->setObjectName("primaryButton");

    addBar->addWidget(m_titleEdit, 1);
    addBar->addWidget(addButton);
    m_body->addLayout(addBar);

    connect(addButton, &QPushButton::clicked, this, &TaskPanel::addTask);
    connect(m_titleEdit, &QLineEdit::returnPressed, this, &TaskPanel::addTask);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName("panelScroll");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_list = new QWidget(m_scroll);
    m_listLayout = new QVBoxLayout(m_list);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(8);
    m_listLayout->addStretch();
    m_scroll->setWidget(m_list);

    m_body->addWidget(m_scroll, 1);
}

void TaskPanel::setProject(qint64 projectId)
{
    if (m_projectId == projectId)
        return;
    m_projectId = projectId;
    reload();
}

void TaskPanel::reload()
{
    TaskService service;
    const QVector<TaskRow> tasks = service.listForProject(m_projectId);

    // drop the old rows, keep the trailing stretch
    while (m_listLayout->count() > 1) {
        QLayoutItem* item = m_listLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    if (tasks.isEmpty()) {
        auto* empty = new QLabel("No tasks in this project yet.", m_list);
        empty->setObjectName("emptyHint");
        empty->setWordWrap(true);
        m_listLayout->insertWidget(0, empty);
        return;
    }

    for (const TaskRow& task : tasks) {
        auto* row = new QFrame(m_list);
        row->setObjectName("itemRow");
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 8, 12, 8);
        rowLayout->setSpacing(8);

        auto* titles = new QVBoxLayout();
        titles->setSpacing(2);

        auto* titleLabel = new QLabel(task.title, row);
        titleLabel->setObjectName("rowTitle");
        titleLabel->setWordWrap(true);
        titles->addWidget(titleLabel);

        const QString detail = taskDetail(task);
        if (!detail.isEmpty()) {
            auto* detailLabel = new QLabel(detail, row);
            detailLabel->setObjectName("rowDetail");
            detailLabel->setWordWrap(true);
            titles->addWidget(detailLabel);
        }

        rowLayout->addLayout(titles, 1);

        auto* status = new QComboBox(row);
        status->setObjectName("rowCombo");
        status->addItem(statusLabel("todo"), "todo");
        status->addItem(statusLabel("in_progress"), "in_progress");
        status->addItem(statusLabel("completed"), "completed");
        const int statusIndex = status->findData(task.status);
        {
            QSignalBlocker blocker(status);
            status->setCurrentIndex(statusIndex >= 0 ? statusIndex : 0);
        }
        connect(status, &QComboBox::currentIndexChanged, this, [this, task, status](int index) {
            TaskService service;
            if (service.setStatus(task.id, status->itemData(index).toString())) {
                reload();
                emit dataChanged();
            }
        });
        rowLayout->addWidget(status);

        auto* editButton = new QPushButton("Edit", row);
        editButton->setObjectName("rowButton");
        connect(editButton, &QPushButton::clicked, this, [this, task, row]() {
            TaskEditDialog dialog(task, row);
            if (dialog.exec() != QDialog::Accepted)
                return;
            TaskService service;
            if (service.update(task.id, dialog.title(), dialog.contents(),
                               dialog.priority(), dialog.dueTime(),
                               dialog.assignedTo())) {
                reload();
                emit dataChanged();
            }
        });
        rowLayout->addWidget(editButton);

        auto* deleteButton = new QPushButton("\u00d7", row);
        deleteButton->setObjectName("rowButton");
        connect(deleteButton, &QPushButton::clicked, this, [this, task]() {
            TaskService service;
            if (service.remove(task.id)) {
                reload();
                emit dataChanged();
            }
        });
        rowLayout->addWidget(deleteButton);

        m_listLayout->insertWidget(m_listLayout->count() - 1, row);
    }
}

void TaskPanel::addTask()
{
    const QString title = m_titleEdit->text().trimmed();
    if (title.isEmpty() || m_projectId <= 0)
        return;

    TaskService service;
    if (service.create(m_projectId, title, QString(), "normal", QDateTime(),
                       0, m_userId)
        > 0) {
        m_titleEdit->clear();
        reload();
        emit dataChanged();
    }
}
