#include "src/ui/settings/SettingsScreen.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "src/data/auth/AuthService.hpp"
#include "src/services/audit/AuditService.hpp"
#include "src/services/settings/SettingsService.hpp"
#include "src/ui/styles/Theme.hpp"
#include "src/ui/widgets/Widgets.hpp"

SettingsScreen::SettingsScreen(const AuthUser& user, QWidget* parent)
    : QWidget(parent)
    , m_user(user)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* heading = new QLabel("Settings", this);
    heading->setObjectName("pageHeading");

    auto* appearance = new Panel("Appearance", this);
    auto* appearanceForm = new QFormLayout();
    appearanceForm->setSpacing(10);

    m_themeBox = new QComboBox(appearance);
    m_themeBox->addItem("System", "system");
    m_themeBox->addItem("Dark", "dark");
    m_themeBox->addItem("Light", "light");
    m_themeBox->setMinimumWidth(200);
    appearanceForm->addRow("Theme", m_themeBox);

    m_timezoneBox = new QComboBox(appearance);
    m_timezoneBox->setMinimumWidth(200);
    for (int offsetHours = -12; offsetHours <= 14; ++offsetHours) {
        const QString label = offsetHours == 0
            ? QStringLiteral("UTC")
            : QStringLiteral("UTC%1%2")
                  .arg(offsetHours > 0 ? QLatin1Char('+') : QLatin1Char('-'))
                  .arg(qAbs(offsetHours));
        m_timezoneBox->addItem(label, label);
    }
    appearanceForm->addRow("Timezone", m_timezoneBox);
    appearance->bodyLayout()->addLayout(appearanceForm);

    auto* timezoneHint = new QLabel(
        "Used for the due times shown on countdowns. Half-hour zones aren't listed yet.",
        appearance);
    timezoneHint->setObjectName("settingsHint");
    timezoneHint->setWordWrap(true);
    appearance->bodyLayout()->addWidget(timezoneHint);

    auto* account = new Panel("Account", this);
    auto* accountForm = new QFormLayout();
    accountForm->setSpacing(10);

    m_notifications = new QCheckBox("Enable notifications", account);
    accountForm->addRow(QString(), m_notifications);

    m_currentPassword = new QLineEdit(account);
    m_currentPassword->setEchoMode(QLineEdit::Password);
    m_currentPassword->setPlaceholderText("Current password");
    accountForm->addRow("Current", m_currentPassword);

    m_newPassword = new QLineEdit(account);
    m_newPassword->setEchoMode(QLineEdit::Password);
    m_newPassword->setPlaceholderText("New password");
    accountForm->addRow("New", m_newPassword);

    m_confirmPassword = new QLineEdit(account);
    m_confirmPassword->setEchoMode(QLineEdit::Password);
    m_confirmPassword->setPlaceholderText("Repeat the new password");
    accountForm->addRow("Repeat", m_confirmPassword);

    auto* passwordButton = new QPushButton("Change password", account);
    passwordButton->setObjectName("primaryButton");
    connect(passwordButton, &QPushButton::clicked, this, &SettingsScreen::changePassword);
    accountForm->addRow(QString(), passwordButton);

    m_passwordHint = new QLabel(account);
    m_passwordHint->setObjectName("settingsHint");
    m_passwordHint->setWordWrap(true);
    accountForm->addRow(QString(), m_passwordHint);

    account->bodyLayout()->addLayout(accountForm);

    auto* saveRow = new QHBoxLayout();
    auto* saveButton = new QPushButton("Save settings", this);
    saveButton->setObjectName("primaryButton");
    connect(saveButton, &QPushButton::clicked, this, &SettingsScreen::save);

    m_savedHint = new QLabel(this);
    m_savedHint->setObjectName("settingsHint");

    saveRow->addWidget(saveButton);
    saveRow->addWidget(m_savedHint);
    saveRow->addStretch();

    layout->addWidget(heading);
    layout->addWidget(appearance);
    layout->addWidget(account);

    // day 9: only admins see the users + audit sections
    if (m_user.role == "admin")
        buildAdminSection(this, layout);

    layout->addLayout(saveRow);
    layout->addStretch();

    load();
}

void SettingsScreen::buildAdminSection(QWidget* parent, QVBoxLayout* layout)
{
    auto* usersPanel = new Panel("Users (admin)", parent);
    m_adminUsersLayout = new QVBoxLayout();
    m_adminUsersLayout->setSpacing(6);
    usersPanel->bodyLayout()->addLayout(m_adminUsersLayout);

    auto* usersHint = new QLabel(
        "Enable/disable and role changes take effect immediately; "
        "disabling a user also ends their sessions.", usersPanel);
    usersHint->setObjectName("settingsHint");
    usersHint->setWordWrap(true);
    usersPanel->bodyLayout()->addWidget(usersHint);

    auto* auditPanel = new Panel("Recent activity", parent);
    m_auditLayout = new QVBoxLayout();
    m_auditLayout->setSpacing(4);
    auditPanel->bodyLayout()->addLayout(m_auditLayout);

    layout->addWidget(usersPanel);
    layout->addWidget(auditPanel);

    reloadAdminUsers();
    reloadAuditLog();
}

void SettingsScreen::reloadAdminUsers()
{
    // wipe the old rows
    while (QLayoutItem* item = m_adminUsersLayout->takeAt(0)) {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    AuthService auth;
    const QVector<UserRow> users = auth.listUsers();

    for (const UserRow& user : users) {
        auto* row = new QFrame(this);
        auto* line = new QHBoxLayout(row);
        line->setContentsMargins(0, 2, 0, 2);
        line->setSpacing(8);

        auto* name = new QLabel(
            user.username
                + (user.displayName.isEmpty() || user.displayName == user.username
                       ? QString()
                       : " (" + user.displayName + ")"),
            row);
        name->setMinimumWidth(140);
        line->addWidget(name);

        auto* roleBox = new QComboBox(row);
        roleBox->addItem("user", "user");
        roleBox->addItem("admin", "admin");
        roleBox->setCurrentText(user.role);
        const qint64 targetId = user.id;
        connect(roleBox, &QComboBox::currentTextChanged, this,
                [this, targetId](const QString& role) {
                    QString error;
                    AuthService service;
                    if (!service.setUserRole(targetId, role, m_user.id, &error))
                        reloadAdminUsers();
                });
        line->addWidget(roleBox);

        auto* activeBox = new QCheckBox("active", row);
        activeBox->setChecked(user.active);
        // you can't disable yourself - grey the box out instead of failing later
        if (targetId == m_user.id)
            activeBox->setEnabled(false);
        connect(activeBox, &QCheckBox::toggled, this,
                [this, targetId, activeBox](bool active) {
                    QString error;
                    AuthService service;
                    if (!service.setUserActive(targetId, active, m_user.id, &error)) {
                        activeBox->setChecked(!active);
                        reloadAdminUsers();
                    }
                });
        line->addWidget(activeBox);

        auto* lastLogin = new QLabel(
            user.lastLoginAt.isValid()
                ? "last login " + user.lastLoginAt.toLocalTime().toString("yyyy-MM-dd HH:mm")
                : "never logged in",
            row);
        lastLogin->setObjectName("settingsHint");
        line->addWidget(lastLogin, 1);

        m_adminUsersLayout->addWidget(row);
    }

    if (users.isEmpty()) {
        auto* empty = new QLabel("no users found.", this);
        empty->setObjectName("settingsHint");
        m_adminUsersLayout->addWidget(empty);
    }
}

void SettingsScreen::reloadAuditLog()
{
    while (QLayoutItem* item = m_auditLayout->takeAt(0)) {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    const QVector<AuditRow> rows = AuditService::recent(20);

    for (const AuditRow& row : rows) {
        auto* label = new QLabel(
            row.createdAt.toLocalTime().toString("MM-dd HH:mm") + "  "
                + row.username + "  " + row.action
                + (row.metadata.isEmpty() ? QString() : "  " + row.metadata),
            this);
        label->setObjectName("settingsHint");
        label->setWordWrap(true);
        m_auditLayout->addWidget(label);
    }

    if (rows.isEmpty()) {
        auto* empty = new QLabel("nothing recorded yet.", this);
        empty->setObjectName("settingsHint");
        m_auditLayout->addWidget(empty);
    }
}

void SettingsScreen::load()
{
    const SettingsRow settings = SettingsService::get(m_user.id);

    const int themeIndex = m_themeBox->findData(settings.theme);
    m_themeBox->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);

    const int zoneIndex = m_timezoneBox->findData(settings.timezone);
    m_timezoneBox->setCurrentIndex(zoneIndex >= 0 ? zoneIndex : 0);

    m_notifications->setChecked(settings.notificationsEnabled);
}

void SettingsScreen::save()
{
    SettingsRow settings;
    settings.userId = m_user.id;
    settings.notificationsEnabled = m_notifications->isChecked();
    settings.theme = m_themeBox->currentData().toString();
    settings.timezone = m_timezoneBox->currentData().toString();

    if (!SettingsService::save(m_user.id, settings)) {
        m_savedHint->setText("Could not save - is the database reachable?");
        return;
    }

    // refresh the in-memory copy (timezone formatting) and restyle live
    SettingsService::loadCurrent(m_user.id);
    OrbitTheme::apply(settings.theme);

    m_savedHint->setText("Saved.");
}

void SettingsScreen::changePassword()
{
    AuthService auth;
    QString error;
    if (!auth.changePassword(m_user.id, m_currentPassword->text(),
                             m_newPassword->text(), m_confirmPassword->text(),
                             &error)) {
        m_passwordHint->setText(error);
        return;
    }

    m_currentPassword->clear();
    m_newPassword->clear();
    m_confirmPassword->clear();
    m_passwordHint->setText("Password changed. Other sessions were signed out.");
}
