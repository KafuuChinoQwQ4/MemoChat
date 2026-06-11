#ifndef MAINQMLBOOTSTRAP_H
#define MAINQMLBOOTSTRAP_H

#include <QUrl>

class QApplication;
class QQmlApplicationEngine;
class AppComposition;

void registerMemoChatQmlTypes();
QUrl memoChatMainQmlUrl();
void configureMemoChatEngine(QQmlApplicationEngine& engine, AppComposition& composition);
bool loadMemoChatMainWindow(QQmlApplicationEngine& engine, QApplication& app, AppComposition& composition);

#endif // MAINQMLBOOTSTRAP_H
