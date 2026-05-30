#ifndef MAINLOGGING_H
#define MAINLOGGING_H

#include <QtGlobal>

class QString;

void loadRuntimeLogConfig(const QString& configPath, const QString& appPath);
void fileMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

#endif // MAINLOGGING_H
