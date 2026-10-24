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
#include <QSpinBox>
#include <QVBoxLayout>

#include "src/data/auth/AuthService.hpp"
#include "src/services/audit/AuditService.hpp"
#include "src/services/settings/SettingsService.hpp"
#include "src/services/share/ShareService.hpp"
#include "src/ui/styles/Theme.hpp"
#include "src/ui/widgets/Widgets.hpp"

namespace {

// one shared shape for every form on this page: same label column, same
// field width, same row spacing - so the panels line up down the page
QFormLayout* makeForm()
{
    auto* form = new QFormLayout();
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignTop);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(10);
    return form;
}

// checkboxes sit in the field column, not under the labels, so their
// left edge lines up with the combo boxes and inputs above them
void addCheckRow(QFormLayout* form, const QString& label, QCheckBox* check)
{
    form->addRow(label, check);
}

}

SettingsScreen::SettingsScreen(const AuthUser& user, QWidget* parent)
    : QWidget(parent)
    , m_user(user)
{
    auto* layout = new QVBoxLayout(this);
    // padding comes from the main window's content layout - keeps this
    // page on the same grid as the dashboard and projects
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    auto* heading = new QLabel("Settings", this);
    heading->setObjectName("pageHeading");
    layout->addWidget(heading);

    // ---- appearance -------------------------------------------------
    auto* appearance = new Panel("Appearance", this);
    auto* appearanceForm = makeForm();

    m_themeBox = new QComboBox(appearance);
    m_themeBox->addItem("System", "system");
    m_themeBox->addItem("Dark", "dark");
    m_themeBox->addItem("Light", "light");
    m_themeBox->setMinimumWidth(220);
    appearanceForm->addRow("Theme", m_themeBox);

    m_timezoneBox = new QComboBox(appearance);
    m_timezoneBox->setMinimumWidth(220);
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
    layout->addWidget(appearance);

    // ---- notifications ----------------------------------------------
    auto* notifications = new Panel("Notifications", this);
    auto* notificationsForm = makeForm();

    m_notifications = new QCheckBox("Desktop notifications", notifications);
    addCheckRow(notificationsForm, "Enable", m_notifications);

    m_notifyLead = new QSpinBox(notifications);
    m_notifyLead->setRange(1, 240);
    m_notifyLead->setSuffix(" min before");
    m_notifyLead->setMinimumWidth(220);
    notificationsForm->addRow("Remind me", m_notifyLead);

    notifications->bodyLayout()->addLayout(notificationsForm);

    auto* notifyHint = new QLabel(
        "Orbit pops a notification when a countdown is due within the lead "
        "time, or a task is due/overdue. Notifications need the tray icon, "
        "which appears while you're logged in.",
        notifications);
    notifyHint->setObjectName("settingsHint");
    notifyHint->setWordWrap(true);
    notifications->bodyLayout()->addWidget(notifyHint);
    layout->addWidget(notifications);

    // ---- window -----------------------------------------------------
    auto* windowPanel = new Panel("Window", this);
    auto* windowForm = makeForm();

    m_alwaysOnTop = new QCheckBox("Keep Orbit above other windows", windowPanel);
    addCheckRow(windowForm, "Always on top", m_alwaysOnTop);

    m_minimizeToTray = new QCheckBox("Hide to the tray icon when closing", windowPanel);
    addCheckRow(windowForm, "Minimize to tray", m_minimizeToTray);

    windowPanel->bodyLayout()->addLayout(windowForm);

    auto* windowHint = new QLabel(
        "With minimize-to-tray, closing the window keeps Orbit running in "
        "the background - open it again from the tray icon, or Quit from "
        "the tray menu.",
        windowPanel);
    windowHint->setObjectName("settingsHint");
    windowHint->setWordWrap(true);
    windowPanel->bodyLayout()->addWidget(windowHint);
    layout->addWidget(windowPanel);

    // ---- sharing (day 12) -------------------------------------------
    auto* sharing = new Panel("Sharing", this);
    auto* sharingForm = makeForm();

    m_shareEnabled = new QCheckBox("Receive files from your phone", sharing);
    addCheckRow(sharingForm, "Local share", m_shareEnabled);

    m_shareUrl = new QLabel(sharing);
    m_shareUrl->setObjectName("settingsHint");
    m_shareUrl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_shareUrl->setWordWrap(true);
    sharingForm->addRow("Link", m_shareUrl);

    sharing->bodyLayout()->addLayout(sharingForm);

    auto* sharingHint = new QLabel(
        "Starts a tiny server on this computer. Open the link on your phone "
        "(same wi-fi) to push a file straight into a project. The link "
        "includes a random token and only works while this computer is "
        "logged in - share it with care, anyone with the link can upload.",
        sharing);
    sharingHint->setObjectName("settingsHint");
    sharingHint->setWordWrap(true);
    sharing->bodyLayout()->addWidget(sharingHint);
    layout->addWidget(sharing);

    // ---- account ----------------------------------------------------
    auto* account = new Panel("Account", this);
    auto* accountForm = makeForm();

    m_currentPassword = new QLineEdit(account);
    m_currentPassword->setEchoMode(QLineEdit::Password);
    m_currentPassword->setPlaceholderText("Current password");
    m_currentPassword->setMinimumWidth(220);
    accountForm->addRow("Current", m_currentPassword);

    m_newPassword = new QLineEdit(account);
    m_newPassword->setEchoMode(QLineEdit::Password);
    m_newPassword->setPlaceholderText("New password");
    m_newPassword->setMinimumWidth(220);
    accountForm->addRow("New", m_newPassword);

    m_confirmPassword = new QLineEdit(account);
    m_confirmPassword->setEchoMode(QLineEdit::Password);
    m_confirmPassword->setPlaceholderText("Repeat the new password");
    m_confirmPassword->setMinimumWidth(220);
    accountForm->addRow("Repeat", m_confirmPassword);

    auto* passwordButton = new QPushButton("Change password", account);
    passwordButton->setObjectName("primaryButton");
    passwordButton->setMaximumWidth(220);
    connect(passwordButton, &QPushButton::clicked, this, &SettingsScreen::changePassword);
    accountForm->addRow(QString(), passwordButton);

    m_passwordHint = new QLabel(account);
    m_passwordHint->setObjectName("settingsHint");
    m_passwordHint->setWordWrap(true);
    accountForm->addRow(QString(), m_passwordHint);

    account->bodyLayout()->addLayout(accountForm);
    layout->addWidget(account);

    // day 9: only admins see the users + audit sections
    if (m_user.role == "admin")
        buildAdminSection(this, layout);

    // ---- save row ---------------------------------------------------
    auto* saveRow = new QHBoxLayout();
    auto* saveButton = new QPushButton("Save settings", this);
    saveButton->setObjectName("primaryButton");
    saveButton->setMaximumWidth(220);
    connect(saveButton, &QPushButton::clicked, this, &SettingsScreen::save);

    m_savedHint = new QLabel(this);
    m_savedHint->setObjectName("settingsHint");

    saveRow->addWidget(saveButton);
    saveRow->addWidget(m_savedHint);
    saveRow->addStretch();

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
        line->setSpacing(12);

        auto* name = new QLabel(
            user.username
                + (user.displayName.isEmpty() || user.displayName == user.username
                       ? QString()
                       : " (" + user.displayName + ")"),
            row);
        name->setFixedWidth(160);
        line->addWidget(name);

        auto* roleBox = new QComboBox(row);
        roleBox->addItem("user", "user");
        roleBox->addItem("admin", "admin");
        roleBox->setCurrentText(user.role);
        roleBox->setMinimumWidth(120);
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
    m_notifyLead->setValue(
        settings.notifyMinutesBefore > 0 ? settings.notifyMinutesBefore : 15);
    m_alwaysOnTop->setChecked(settings.alwaysOnTop);
    m_minimizeToTray->setChecked(settings.minimizeToTray);
    m_shareEnabled->setChecked(settings.shareEnabled);

    refreshShareUrl();
}

void SettingsScreen::save()
{
    SettingsRow settings;
    settings.userId = m_user.id;
    settings.notificationsEnabled = m_notifications->isChecked();
    settings.theme = m_themeBox->currentData().toString();
    settings.timezone = m_timezoneBox->currentData().toString();
    settings.notifyMinutesBefore = m_notifyLead->value();
    settings.alwaysOnTop = m_alwaysOnTop->isChecked();
    settings.minimizeToTray = m_minimizeToTray->isChecked();
    settings.shareEnabled = m_shareEnabled->isChecked();

    if (!SettingsService::save(m_user.id, settings)) {
        m_savedHint->setText("Could not save - is the database reachable?");
        return;
    }

    // bring the share server in line with the new setting right away
    // instead of waiting for the next login
    bool shareFailed = false;
    auto& share = ShareService::instance();
    if (settings.shareEnabled && !share.isRunning())
        shareFailed = !share.start(m_user.id);
    else if (!settings.shareEnabled && share.isRunning())
        share.stop();
    refreshShareUrl();

    // refresh the in-memory copy (timezone formatting, tray decisions)
    // and restyle live
    SettingsService::loadCurrent(m_user.id);
    OrbitTheme::apply(settings.theme);

    m_savedHint->setText(shareFailed
                             ? QStringLiteral(
                                   "Saved, but the share server could not bind a port.")
                             : QStringLiteral("Saved."));
    emit settingsChanged();
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

void SettingsScreen::refreshShareUrl()
{
    auto& share = ShareService::instance();
    if (!share.isRunning()) {
        m_shareUrl->setText(QStringLiteral("off - turn on local share to get a link"));
        return;
    }

    const QString url = share.url();
    m_shareUrl->setText(url.isEmpty()
                            ? QStringLiteral(
                                  "running, but no usable network address was found")
                            : url);
}
