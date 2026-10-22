#include "src/ui/styles/Theme.hpp"

#include <QApplication>
#include <QFile>
#include <QSettings>

namespace OrbitTheme {
namespace {

QString loadSheet(const char* resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}

bool systemPrefersLight()
{
    // windows stores the per-app theme preference here; 1 = light
    QSettings registry(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    return registry.value("AppsUseLightTheme", 1).toBool();
}

}

void apply(const QString& preference)
{
    if (!qApp)
        return;

    const bool light = preference == "light"
        || (preference != "dark" && systemPrefersLight());

    qApp->setStyleSheet(
        loadSheet(light ? ":/styles/ThemeLight.qss" : ":/styles/Theme.qss"));
}

}
