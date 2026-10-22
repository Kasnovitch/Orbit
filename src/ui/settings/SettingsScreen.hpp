#pragma once

#include <QWidget>

#include "src/data/auth/AuthService.hpp"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QVBoxLayout;

class SettingsScreen : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsScreen(const AuthUser& user, QWidget* parent = nullptr);

private:
    void load();
    void save();
    void changePassword();

    // day 9: admin-only users list + recent audit entries
    void buildAdminSection(QWidget* parent, QVBoxLayout* layout);
    void reloadAdminUsers();
    void reloadAuditLog();

    AuthUser m_user;
    QComboBox* m_themeBox = nullptr;
    QComboBox* m_timezoneBox = nullptr;
    QCheckBox* m_notifications = nullptr;
    QLineEdit* m_currentPassword = nullptr;
    QLineEdit* m_newPassword = nullptr;
    QLineEdit* m_confirmPassword = nullptr;
    QLabel* m_savedHint = nullptr;
    QLabel* m_passwordHint = nullptr;
    QVBoxLayout* m_adminUsersLayout = nullptr;
    QVBoxLayout* m_auditLayout = nullptr;
};
