#ifndef MAINWINDOWEFFECTS_H
#define MAINWINDOWEFFECTS_H

#include <QtGlobal>

class QQuickWindow;

#ifdef Q_OS_WIN
void ensureAcrylicVisibleHook(QQuickWindow* window);
void applyAcrylicToTopLevelQuickWindows();
#endif

#endif // MAINWINDOWEFFECTS_H
