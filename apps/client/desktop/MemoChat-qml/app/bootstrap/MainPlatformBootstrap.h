#ifndef MAINPLATFORMBOOTSTRAP_H
#define MAINPLATFORMBOOTSTRAP_H

#include <QtGlobal>

#ifdef Q_OS_LINUX
void configureLinuxRendering();
void configureLinuxInputMethod();
#endif

#endif // MAINPLATFORMBOOTSTRAP_H
