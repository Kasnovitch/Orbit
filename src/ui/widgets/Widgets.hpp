#pragma once

#include <QFrame>

class QLabel;
class QVBoxLayout;

class StatTile : public QFrame
{
    Q_OBJECT
public:
    explicit StatTile(const QString& title, QWidget* parent = nullptr);
    void setValue(const QString& value);

private:
    QLabel* m_value = nullptr;
};

class Panel : public QFrame
{
    Q_OBJECT
public:
    explicit Panel(const QString& title, QWidget* parent = nullptr);
    QVBoxLayout* bodyLayout() const { return m_body; }

protected:
    QVBoxLayout* m_body = nullptr;
};
