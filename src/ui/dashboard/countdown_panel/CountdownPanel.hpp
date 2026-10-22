#pragma once

#include <QDateTime>
#include <QHash>

#include "src/ui/widgets/Widgets.hpp"

class QCheckBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QScrollArea;
class QTimer;
class QVBoxLayout;

class CountdownPanel : public Panel
{
    Q_OBJECT
public:
    explicit CountdownPanel(qint64 userId, QWidget* parent = nullptr);

    void setProject(qint64 projectId);

signals:
    void dataChanged();

private:
    void reload();
    void addCountdown();
    void tick(); // refresh the remaining labels from the real due times

    qint64 m_userId = 0;
    qint64 m_projectId = 0;
    QLineEdit* m_titleEdit = nullptr;
    QDateTimeEdit* m_dueEdit = nullptr;
    QCheckBox* m_globalCheck = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_list = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
    QTimer* m_timer = nullptr;

    // row widgets keyed by countdown id so the 1s tick can update them
    QHash<qint64, QDateTime> m_dueTimes;
    QHash<qint64, QLabel*> m_remainingLabels;
};
