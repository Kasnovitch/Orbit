#include "src/ui/dashboard/links_panel/LinksPanel.hpp"

#include <QDesktopServices>
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

#include "src/services/links/LinkService.hpp"

namespace {

class LinkEditDialog : public QDialog
{
public:
    explicit LinkEditDialog(const LinkRow& link, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Edit link");
        auto* form = new QFormLayout(this);

        m_title = new QLineEdit(link.title, this);
        m_url = new QLineEdit(link.url, this);

        form->addRow("Title", m_title);
        form->addRow("Url", m_url);

        auto* buttons =
            new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        form->addRow(buttons);
    }

    QString title() const { return m_title->text().trimmed(); }
    QString url() const { return m_url->text().trimmed(); }

private:
    QLineEdit* m_title = nullptr;
    QLineEdit* m_url = nullptr;
};

}

LinksPanel::LinksPanel(qint64 userId, QWidget* parent)
    : Panel("Links", parent)
    , m_userId(userId)
{
    auto* addBar = new QHBoxLayout();

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setObjectName("panelInput");
    m_titleEdit->setPlaceholderText("Title\u2026");

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setObjectName("panelInput");
    m_urlEdit->setPlaceholderText("https://\u2026");

    auto* addButton = new QPushButton("Add", this);
    addButton->setObjectName("primaryButton");

    addBar->addWidget(m_titleEdit, 1);
    addBar->addWidget(m_urlEdit, 2);
    addBar->addWidget(addButton);
    m_body->addLayout(addBar);

    connect(addButton, &QPushButton::clicked, this, &LinksPanel::addLink);
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &LinksPanel::addLink);

    m_list = new QWidget(this);
    m_listLayout = new QVBoxLayout(m_list);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(8);
    m_listLayout->addStretch();
    m_body->addWidget(m_list, 1);
}

void LinksPanel::setProject(qint64 projectId)
{
    if (m_projectId == projectId)
        return;
    m_projectId = projectId;
    reload();
}

void LinksPanel::reload()
{
    LinkService service;
    const QVector<LinkRow> links = service.listForProject(m_projectId);

    // drop the old rows, keep the trailing stretch
    while (m_listLayout->count() > 1) {
        QLayoutItem* item = m_listLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    if (links.isEmpty()) {
        auto* empty = new QLabel("No links on this project yet.", m_list);
        empty->setObjectName("emptyHint");
        empty->setWordWrap(true);
        m_listLayout->insertWidget(0, empty);
        return;
    }

    for (const LinkRow& link : links) {
        auto* row = new QFrame(m_list);
        row->setObjectName("itemRow");
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 8, 12, 8);
        rowLayout->setSpacing(8);

        auto* openButton = new QPushButton(link.title, row);
        openButton->setObjectName("linkButton");
        openButton->setToolTip(link.url);
        connect(openButton, &QPushButton::clicked, this, [link]() {
            QDesktopServices::openUrl(QUrl(link.url));
        });
        rowLayout->addWidget(openButton, 1);

        auto* editButton = new QPushButton("Edit", row);
        editButton->setObjectName("rowButton");
        connect(editButton, &QPushButton::clicked, this, [this, link, row]() {
            LinkEditDialog dialog(link, row);
            if (dialog.exec() != QDialog::Accepted)
                return;

            LinkService service;
            QString error;
            if (!service.update(link.id, dialog.title(), dialog.url(), &error)) {
                QMessageBox::warning(row, "Links", error);
                return;
            }
            reload();
        });
        rowLayout->addWidget(editButton);

        auto* deleteButton = new QPushButton("\u00d7", row);
        deleteButton->setObjectName("rowButton");
        connect(deleteButton, &QPushButton::clicked, this, [this, link]() {
            LinkService service;
            if (service.remove(link.id))
                reload();
        });
        rowLayout->addWidget(deleteButton);

        m_listLayout->insertWidget(m_listLayout->count() - 1, row);
    }
}

void LinksPanel::addLink()
{
    LinkService service;
    QString error;
    if (!service.add(m_projectId, m_titleEdit->text(), m_urlEdit->text(),
                     m_userId, &error)) {
        if (!error.isEmpty() && m_projectId > 0)
            QMessageBox::warning(this, "Links", error);
        return;
    }

    m_titleEdit->clear();
    m_urlEdit->clear();
    reload();
}
