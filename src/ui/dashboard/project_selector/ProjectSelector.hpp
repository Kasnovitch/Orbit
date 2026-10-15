#pragma once

#include <QWidget>

class QComboBox;

class ProjectSelector : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectSelector(QWidget* parent = nullptr);

signals:
    void projectChanged(const QString& projectName);

private:
    QComboBox* m_combo = nullptr;
};
