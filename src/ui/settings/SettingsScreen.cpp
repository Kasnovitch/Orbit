#include "src/ui/settings/SettingsScreen.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "src/data/auth/AuthService.hpp"
#include "src/services/settings/SettingsService.hpp"
#include "src/ui/styles/Theme.hpp"
#include "src/ui/widgets/Widgets.hpp"

SettingsScreen::SettingsScreen(qint64 userId, QWidget* parent)
    : QWidget(parent)
    , m_userId(userId)
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
    layout->addLayout(saveRow);
    layout->addStretch();

    load();
}

void SettingsScreen::load()
{
    const SettingsRow settings = SettingsService::get(m_userId);

    const int themeIndex = m_themeBox->findData(settings.theme);
    m_themeBox->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);

    const int zoneIndex = m_timezoneBox->findData(settings.timezone);
    m_timezoneBox->setCurrentIndex(zoneIndex >= 0 ? zoneIndex : 0);

    m_notifications->setChecked(settings.notificationsEnabled);
}

void SettingsScreen::save()
{
    SettingsRow settings;
    settings.userId = m_userId;
    settings.notificationsEnabled = m_notifications->isChecked();
    settings.theme = m_themeBox->currentData().toString();
    settings.timezone = m_timezoneBox->currentData().toString();

    if (!SettingsService::save(m_userId, settings)) {
        m_savedHint->setText("Could not save - is the database reachable?");
        return;
    }

    // refresh the in-memory copy (timezone formatting) and restyle live
    SettingsService::loadCurrent(m_userId);
    OrbitTheme::apply(settings.theme);

    m_savedHint->setText("Saved.");
}

void SettingsScreen::changePassword()
{
    AuthService auth;
    QString error;
    if (!auth.changePassword(m_userId, m_currentPassword->text(),
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
