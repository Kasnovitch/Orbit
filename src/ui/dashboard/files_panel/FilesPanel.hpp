#pragma once

#include "src/ui/widgets/Widgets.hpp"

class QScrollArea;

class FilesPanel : public Panel
{
    Q_OBJECT
public:
    explicit FilesPanel(qint64 userId, QWidget* parent = nullptr);

    void setProject(qint64 projectId);

    // public so the dashboard can reload us when a phone upload lands
    // through the share server
    void refresh() { reload(); }

private:
    void reload();
    void upload();

    qint64 m_userId = 0;
    qint64 m_projectId = 0;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_list = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
};
