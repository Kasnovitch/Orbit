#include "src/ui/settings/SettingsScreen.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "src/ui/widgets/Widgets.hpp"

SettingsScreen::SettingsScreen(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* heading = new QLabel("Settings", this);
    heading->setObjectName("pageHeading");

    auto* appearance = new Panel("Appearance", this);
    auto* appearanceForm = new QFormLayout();
    appearanceForm->setSpacing(10);

    auto* themeBox = new QComboBox(appearance);
    themeBox->addItems({ "System", "Dark", "Light" });
    themeBox->setEnabled(false);
    appearanceForm->addRow("Theme", themeBox);

    auto* timezoneBox = new QComboBox(appearance);
    timezoneBox->addItem("System default");
    timezoneBox->setEnabled(false);
    appearanceForm->addRow("Timezone", timezoneBox);
    appearance->bodyLayout()->addLayout(appearanceForm);

    auto* account = new Panel("Account", this);
    auto* notify = new QCheckBox("Enable notifications", account);
    notify->setEnabled(false);
    auto* passwordButton = new QPushButton("Change password", account);
    passwordButton->setEnabled(false);
    auto* comingSoon = new QLabel("Wired up once the backend lands.", account);
    comingSoon->setObjectName("emptyHint");

    account->layout()->addWidget(notify);
    account->layout()->addWidget(passwordButton);
    account->layout()->addWidget(comingSoon);

    auto* teamsNote = new QLabel("Teams is a placeholder for now.", this);
    teamsNote->setObjectName("emptyHint");

    layout->addWidget(heading);
    layout->addWidget(appearance);
    layout->addWidget(account);
    layout->addWidget(teamsNote);
    layout->addStretch();
}
