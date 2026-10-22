#include "src/ui/dashboard/countdown_panel/CountdownPanel.hpp"

#include <QCheckBox>
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
#include <QTimer>
#include <QVBoxLayout>

#include "src/services/countdowns/CountdownService.hpp"

namespace {

QString pad2(int value)
{
    return QStringLiteral("%1").arg(value, 2, 10, QLatin1Char('0'));
}

// remaining time straight from the due time - nothing here is cached
QString formatRemaining(const QDateTime& due)
{
    const qint64 secs = QDateTime::currentDateTimeUtc().secsTo(due.toUTC());

    if (secs <= 0) {
        const qint64 overdue = -secs;
        if (overdue >= 86400)
            return QStringLiteral("overdue %1d").arg(overdue / 86400);
        if (overdue >= 3600)
            return QStringLiteral("overdue %1h").arg(overdue / 3600);
        return QStringLiteral("overdue %1m").arg(qMax<qint64>(overdue / 60, 1));
    }

    const qint64 days = secs / 86400;
    const int hours = int((secs % 86400) / 3600);
    const int minutes = int((secs % 3600) / 60);
    const int seconds = int(secs % 60);

    if (days > 0)
        return QStringLiteral("%1d %2:%3").arg(days).arg(pad2(hours)).arg(pad2(minutes));
    return QStringLiteral("%1:%2:%3").arg(pad2(hours)).arg(pad2(minutes)).arg(pad2(seconds));
}

// file local edit dialog: title, description, due time. whether a
// countdown is global is decided when it's created and stays that way
class CountdownEditDialog : public QDialog
{
public:
    explicit CountdownEditDialog(const CountdownRow& row, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Edit countdown");
        auto* form = new QFormLayout(this);

        m_title = new QLineEdit(row.title, this);
        m_description = new QPlainTextEdit(row.description, this);
        m_description->setMinimumHeight(80);
        m_due = new QDateTimeEdit(row.dueTime, this);
        m_due->setCalendarPopup(true);

        form->addRow("Title", m_title);
        form->addRow("Description", m_description);
        form->addRow("Due", m_due);

        auto* buttons =
            new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        form->addRow(buttons);
    }

    QString title() const { return m_title->text().trimmed(); }
    QString description() const { return m_description->toPlainText(); }
    QDateTime dueTime() const { return m_due->dateTime(); }

private:
    QLineEdit* m_title = nullptr;
    QPlainTextEdit* m_description = nullptr;
    QDateTimeEdit* m_due = nullptr;
};

}

CountdownPanel::CountdownPanel(qint64 userId, QWidget* parent)
    : Panel("Countdowns", parent)
    , m_userId(userId)
{
    auto* addBar = new QHBoxLayout();

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setObjectName("panelInput");
    m_titleEdit->setPlaceholderText("New countdown title\u2026");

    m_dueEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(1), this);
    m_dueEdit->setCalendarPopup(true);
    m_dueEdit->setObjectName("panelInput");

    m_globalCheck = new QCheckBox("Global", this);
    m_globalCheck->setToolTip("Global countdowns show for every project");

    auto* addButton = new QPushButton("Add", this);
    addButton->setObjectName("primaryButton");

    addBar->addWidget(m_titleEdit, 1);
    addBar->addWidget(m_dueEdit);
    addBar->addWidget(m_globalCheck);
    addBar->addWidget(addButton);
    m_body->addLayout(addBar);

    connect(addButton, &QPushButton::clicked, this, &CountdownPanel::addCountdown);
    connect(m_titleEdit, &QLineEdit::returnPressed, this, &CountdownPanel::addCountdown);

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

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &CountdownPanel::tick);
    m_timer->start();
}

void CountdownPanel::setProject(qint64 projectId)
{
    if (m_projectId == projectId)
        return;
    m_projectId = projectId;
    reload();
}

void CountdownPanel::reload()
{
    CountdownService service;
    const QVector<CountdownRow> countdowns = service.listForProject(m_projectId);

    m_dueTimes.clear();
    m_remainingLabels.clear();

    // drop the old rows, keep the trailing stretch
    while (m_listLayout->count() > 1) {
        QLayoutItem* item = m_listLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    if (countdowns.isEmpty()) {
        auto* empty = new QLabel("No countdowns yet - add the nearest deadline above.", m_list);
        empty->setObjectName("emptyHint");
        empty->setWordWrap(true);
        m_listLayout->insertWidget(0, empty);
        return;
    }

    for (const CountdownRow& row : countdowns) {
        auto* item = new QFrame(m_list);
        item->setObjectName("itemRow");
        auto* rowLayout = new QHBoxLayout(item);
        rowLayout->setContentsMargins(12, 8, 12, 8);
        rowLayout->setSpacing(8);

        auto* titles = new QVBoxLayout();
        titles->setSpacing(2);

        auto* titleLabel = new QLabel(
            row.projectId > 0 ? row.title : row.title + "  (global)", item);
        titleLabel->setObjectName("rowTitle");
        titleLabel->setWordWrap(true);
        titles->addWidget(titleLabel);

        auto* remainingLabel = new QLabel(formatRemaining(row.dueTime), item);
        remainingLabel->setObjectName("rowRemaining");
        titles->addWidget(remainingLabel);

        rowLayout->addLayout(titles, 1);

        auto* editButton = new QPushButton("Edit", item);
        editButton->setObjectName("rowButton");
        connect(editButton, &QPushButton::clicked, this, [this, row, item]() {
            CountdownEditDialog dialog(row, item);
            if (dialog.exec() != QDialog::Accepted)
                return;
            CountdownService service;
            if (service.update(row.id, dialog.title(), dialog.description(),
                               dialog.dueTime())) {
                reload();
                emit dataChanged();
            }
        });
        rowLayout->addWidget(editButton);

        auto* deleteButton = new QPushButton("\u00d7", item);
        deleteButton->setObjectName("rowButton");
        connect(deleteButton, &QPushButton::clicked, this, [this, row]() {
            CountdownService service;
            if (service.remove(row.id)) {
                reload();
                emit dataChanged();
            }
        });
        rowLayout->addWidget(deleteButton);

        m_listLayout->insertWidget(m_listLayout->count() - 1, item);
        m_dueTimes.insert(row.id, row.dueTime);
        m_remainingLabels.insert(row.id, remainingLabel);
    }
}

void CountdownPanel::addCountdown()
{
    const QString title = m_titleEdit->text().trimmed();
    const QDateTime due = m_dueEdit->dateTime();
    if (title.isEmpty() || !due.isValid())
        return;

    const qint64 projectId = m_globalCheck->isChecked() ? 0 : m_projectId;

    CountdownService service;
    if (service.create(projectId, title, QString(), due, m_userId) > 0) {
        m_titleEdit->clear();
        reload();
        emit dataChanged();
    }
}

void CountdownPanel::tick()
{
    for (auto it = m_dueTimes.constBegin(); it != m_dueTimes.constEnd(); ++it) {
        if (QLabel* label = m_remainingLabels.value(it.key(), nullptr))
            label->setText(formatRemaining(it.value()));
    }
}
