#ifndef MAINRUNTIMECONFIG_H
#define MAINRUNTIMECONFIG_H

#include <QString>

QString resolveStartupAppPath(const char* argv0);
QString configPathForAppPath(const QString& appPath);
void configureGateUrlPrefixes(const QString& configPath);

#endif // MAINRUNTIMECONFIG_H
