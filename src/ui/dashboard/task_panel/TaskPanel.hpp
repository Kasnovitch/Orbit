#pragma once

#include "src/ui/widgets/Widgets.hpp"

class QComboBox;
class QLineEdit;
class QScrollArea;
class QVBoxLayout;

class TaskPanel : public Panel
{
    Q_OBJECT
public:
    explicit TaskPanel(qint64 userId, QWidget* parent = nullptr);

    void setProject(qint64 projectId);

signals:
    void dataChanged();

private:
    void reload();
    void addTask();

    qint64 m_userId = 0;
    qint64 m_projectId = 0;
    QLineEdit* m_titleEdit = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_list = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
};
