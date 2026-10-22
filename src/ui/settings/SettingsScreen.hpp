#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;

class SettingsScreen : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsScreen(qint64 userId, QWidget* parent = nullptr);

private:
    void load();
    void save();
    void changePassword();

    qint64 m_userId = 0;
    QComboBox* m_themeBox = nullptr;
    QComboBox* m_timezoneBox = nullptr;
    QCheckBox* m_notifications = nullptr;
    QLineEdit* m_currentPassword = nullptr;
    QLineEdit* m_newPassword = nullptr;
    QLineEdit* m_confirmPassword = nullptr;
    QLabel* m_savedHint = nullptr;
    QLabel* m_passwordHint = nullptr;
};
