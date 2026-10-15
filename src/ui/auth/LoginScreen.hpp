#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;

class LoginScreen : public QWidget
{
    Q_OBJECT
public:
    explicit LoginScreen(QWidget* parent = nullptr);

    void setDatabaseStatus(const QString& text);
    void showNote(const QString& text);
    void clearPassword();

signals:
    void loginAttempted(const QString& username, const QString& password, bool remember);
    void registerAttempted(const QString& username, const QString& password,
                           const QString& confirmPassword, bool remember);

private:
    void attemptLogin();
    void switchMode();

    QLineEdit* m_username = nullptr;
    QLineEdit* m_password = nullptr;
    QLineEdit* m_confirm = nullptr;
    QCheckBox* m_remember = nullptr;
    QLabel* m_note = nullptr;
    QPushButton* m_primaryButton = nullptr;
    QPushButton* m_modeButton = nullptr;
    bool m_registerMode = false;
};
