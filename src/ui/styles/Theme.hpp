#pragma once

#include <QString>

// loads the right stylesheet for a preference: "dark", "light" or
// "system" (follows the windows app theme)
namespace OrbitTheme {
void apply(const QString& preference);
}
