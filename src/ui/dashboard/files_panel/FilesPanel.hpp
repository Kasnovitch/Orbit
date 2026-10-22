#pragma once

#include "src/ui/widgets/Widgets.hpp"

class FilesPanel : public Panel
{
    Q_OBJECT
public:
    explicit FilesPanel(qint64 userId, QWidget* parent = nullptr);

    void setProject(qint64 projectId);

private:
    void reload();
    void upload();

    qint64 m_userId = 0;
    qint64 m_projectId = 0;
    QWidget* m_list = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
};
