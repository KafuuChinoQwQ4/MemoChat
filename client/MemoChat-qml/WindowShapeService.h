#ifndef WINDOWSHAPESERVICE_H
#define WINDOWSHAPESERVICE_H

class QWindow;

class WindowShapeService
{
public:
    static void bindRoundedMask(QWindow *window, int radius);
};

#endif // WINDOWSHAPESERVICE_H
