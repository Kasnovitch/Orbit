#pragma once

#include "src/ui/widgets/Widgets.hpp"

class QLineEdit;

class LinksPanel : public Panel
{
    Q_OBJECT
public:
    explicit LinksPanel(qint64 userId, QWidget* parent = nullptr);

    void setProject(qint64 projectId);

private:
    void reload();
    void addLink();

    qint64 m_userId = 0;
    qint64 m_projectId = 0;
    QLineEdit* m_titleEdit = nullptr;
    QLineEdit* m_urlEdit = nullptr;
    QWidget* m_list = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
};
