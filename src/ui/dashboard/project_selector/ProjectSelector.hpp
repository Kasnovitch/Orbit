#pragma once

#include <QWidget>

class QComboBox;

class ProjectSelector : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectSelector(QWidget* parent = nullptr);

    // fills the combo from the database (active projects), keeping the
    // current selection where possible
    void reload();

    qint64 currentProjectId() const;

signals:
    void projectChanged(qint64 projectId);

private:
    QComboBox* m_combo = nullptr;
};
