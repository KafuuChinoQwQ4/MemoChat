#ifndef MAINQMLBOOTSTRAP_H
#define MAINQMLBOOTSTRAP_H

#include <QUrl>

class QApplication;
class QQmlApplicationEngine;
class AppController;

void registerMemoChatQmlTypes();
QUrl memoChatMainQmlUrl();
void configureMemoChatEngine(QQmlApplicationEngine& engine, AppController& controller);
bool loadMemoChatMainWindow(QQmlApplicationEngine& engine, QApplication& app, AppController& controller);

#endif // MAINQMLBOOTSTRAP_H
