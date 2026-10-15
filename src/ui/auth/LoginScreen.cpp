#include "src/ui/auth/LoginScreen.hpp"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginScreen::LoginScreen(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("loginScreen");
    setWindowTitle("Orbit \u2014 Login");
    resize(420, 560);

    auto* page = new QVBoxLayout(this);
    page->setContentsMargins(48, 48, 48, 48);
    page->setSpacing(12);
    page->addStretch();

    auto* title = new QLabel("Orbit", this);
    title->setObjectName("loginTitle");
    title->setAlignment(Qt::AlignCenter);

    auto* subtitle = new QLabel("Project management dashboard", this);
    subtitle->setObjectName("loginSubtitle");
    subtitle->setAlignment(Qt::AlignCenter);

    m_username = new QLineEdit(this);
    m_username->setObjectName("loginInput");
    m_username->setPlaceholderText("Username");

    m_password = new QLineEdit(this);
    m_password->setObjectName("loginInput");
    m_password->setPlaceholderText("Password");
    m_password->setEchoMode(QLineEdit::Password);

    m_confirm = new QLineEdit(this);
    m_confirm->setObjectName("loginInput");
    m_confirm->setPlaceholderText("Confirm password");
    m_confirm->setEchoMode(QLineEdit::Password);
    m_confirm->setVisible(false);

    m_remember = new QCheckBox("Remember me", this);
    m_remember->setObjectName("rememberCheck");

    m_primaryButton = new QPushButton("Log in", this);
    m_primaryButton->setObjectName("primaryButton");
    m_primaryButton->setDefault(true);

    m_modeButton = new QPushButton("Don't have an account? Register", this);
    m_modeButton->setObjectName("linkButton");
    m_modeButton->setFlat(true);

    // register and remember me don't exist until day 2, keep the
    // day 1 login page plain
    m_remember->setVisible(false);
    m_modeButton->setVisible(false);

    m_note = new QLabel("login is a placeholder for now \u2014 any details will let you in.", this);
    m_note->setObjectName("loginNote");
    m_note->setAlignment(Qt::AlignCenter);
    m_note->setWordWrap(true);

    page->addWidget(title);
    page->addWidget(subtitle);
    page->addSpacing(24);
    page->addWidget(m_username);
    page->addWidget(m_password);
    page->addWidget(m_confirm);
    page->addWidget(m_remember);
    page->addWidget(m_primaryButton);
    page->addWidget(m_modeButton);
    page->addSpacing(8);
    page->addWidget(m_note);
    page->addStretch();

    connect(m_primaryButton, &QPushButton::clicked, this, &LoginScreen::attemptLogin);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginScreen::attemptLogin);
    connect(m_confirm, &QLineEdit::returnPressed, this, &LoginScreen::attemptLogin);
    connect(m_modeButton, &QPushButton::clicked, this, &LoginScreen::switchMode);
}

void LoginScreen::setDatabaseStatus(const QString& text)
{
    m_note->setText(text);
}

void LoginScreen::showNote(const QString& text)
{
    m_note->setText(text);
}

void LoginScreen::clearPassword()
{
    m_password->clear();
    m_confirm->clear();
    m_username->setFocus();
}

void LoginScreen::attemptLogin()
{
    const QString username = m_username->text().trimmed();
    if (username.isEmpty()) {
        m_username->setFocus();
        return;
    }
    if (m_password->text().isEmpty()) {
        m_password->setFocus();
        return;
    }

    if (m_registerMode) {
        emit registerAttempted(username, m_password->text(), m_confirm->text(),
                               m_remember->isChecked());
    } else {
        emit loginAttempted(username, m_password->text(), m_remember->isChecked());
    }
}

void LoginScreen::switchMode()
{
    m_registerMode = !m_registerMode;

    m_confirm->setVisible(m_registerMode);
    m_primaryButton->setText(m_registerMode ? "Create account" : "Log in");
    m_modeButton->setText(m_registerMode
        ? "Already have an account? Log in"
        : "Don't have an account? Register");

    m_note->setText(m_registerMode
        ? "new accounts get the regular user role."
        : "");
    m_username->setFocus();
}
