#include "src/ui/dashboard/files_panel/FilesPanel.hpp"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "src/services/files/FileService.hpp"

namespace {

QString humanSize(qint64 bytes)
{
    if (bytes >= 1024 * 1024)
        return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    if (bytes >= 1024)
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 0);
    return QStringLiteral("%1 B").arg(bytes);
}

}

FilesPanel::FilesPanel(qint64 userId, QWidget* parent)
    : Panel("Files", parent)
    , m_userId(userId)
{
    auto* uploadButton = new QPushButton("Upload\u2026", this);
    uploadButton->setObjectName("primaryButton");
    m_body->addWidget(uploadButton, 0, Qt::AlignLeft);
    connect(uploadButton, &QPushButton::clicked, this, &FilesPanel::upload);

    m_list = new QWidget(this);
    m_listLayout = new QVBoxLayout(m_list);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(8);
    m_listLayout->addStretch();
    m_body->addWidget(m_list, 1);
}

void FilesPanel::setProject(qint64 projectId)
{
    if (m_projectId == projectId)
        return;
    m_projectId = projectId;
    reload();
}

void FilesPanel::reload()
{
    FileService service;
    const QVector<FileRow> files = service.listForProject(m_projectId);

    // drop the old rows, keep the trailing stretch
    while (m_listLayout->count() > 1) {
        QLayoutItem* item = m_listLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    if (files.isEmpty()) {
        auto* empty = new QLabel("No files uploaded to this project yet.", m_list);
        empty->setObjectName("emptyHint");
        empty->setWordWrap(true);
        m_listLayout->insertWidget(0, empty);
        return;
    }

    for (const FileRow& file : files) {
        auto* row = new QFrame(m_list);
        row->setObjectName("itemRow");
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 8, 12, 8);
        rowLayout->setSpacing(8);

        auto* openButton = new QPushButton(file.filename, row);
        openButton->setObjectName("linkButton");
        openButton->setToolTip(file.mimeType);
        connect(openButton, &QPushButton::clicked, this, [this, file]() {
            FileService service;
            QDesktopServices::openUrl(QUrl::fromLocalFile(service.absolutePath(file.storageKey)));
        });
        rowLayout->addWidget(openButton, 1);

        auto* sizeLabel = new QLabel(humanSize(file.sizeBytes), row);
        sizeLabel->setObjectName("rowDetail");
        rowLayout->addWidget(sizeLabel);

        auto* deleteButton = new QPushButton("\u00d7", row);
        deleteButton->setObjectName("rowButton");
        connect(deleteButton, &QPushButton::clicked, this, [this, file, row]() {
            FileService service;
            QString error;
            if (!service.remove(file.id, &error)) {
                QMessageBox::warning(row, "Files", error);
                return;
            }
            reload();
        });
        rowLayout->addWidget(deleteButton);

        m_listLayout->insertWidget(m_listLayout->count() - 1, row);
    }
}

void FilesPanel::upload()
{
    const QString sourcePath = QFileDialog::getOpenFileName(this, "Upload file");
    if (sourcePath.isEmpty())
        return;

    FileService service;
    QString error;
    if (service.upload(sourcePath, m_projectId, m_userId, &error).isEmpty()) {
        if (!error.isEmpty() && m_projectId > 0)
            QMessageBox::warning(this, "Files", error);
        return;
    }
    reload();
}
