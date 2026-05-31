#ifndef MAINWINDOWHOOKS_H
#define MAINWINDOWHOOKS_H

class QObject;
class QQuickWindow;

void ensureInitialCenteringHook(QQuickWindow* window);
void ensureTopLevelQuickWindowHooks();
void scheduleTopLevelQuickWindowHookRetries(QObject* context, int remaining = 6);

#endif // MAINWINDOWHOOKS_H
