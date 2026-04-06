#include "ScreenCaptureService.h"

#include "ScreenCaptureWidget.h"

#include <QApplication>
#include <QScreen>
#include <QWidget>
#include <QDebug>
#include <QGuiApplication>
#include <QPainter>

#include <windows.h>
#include <vector>

static QPixmap grabScreenRect(const QRect &rect)
{
    const int w = rect.width();
    const int h = rect.height();
    if (w <= 0 || h <= 0)
        return QPixmap();

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, w, h);
    HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hdcMem, hBitmap));

    BitBlt(hdcMem, 0, 0, w, h, hdcScreen, rect.x(), rect.y(), SRCCOPY);
    SelectObject(hdcMem, hOldBitmap);

    // Extract pixel data via GetDIBits (Qt6 no longer accepts HBITMAP directly)
    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = w;
    bih.biHeight = -h; // top-down
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;

    std::vector<unsigned char> pixels(w * h * 4);
    GetDIBits(hdcMem, hBitmap, 0, h, pixels.data(),
              reinterpret_cast<BITMAPINFO *>(&bih), DIB_RGB_COLORS);

    QPixmap pm = QPixmap::fromImage(
        QImage(pixels.data(), w, h, QImage::Format_ARGB32).copy());

    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    return pm;
}

ScreenCaptureService::ScreenCaptureService(QObject *parent)
    : QObject(parent) {
}

ScreenCaptureService::~ScreenCaptureService() = default;

bool ScreenCaptureService::captureScreen(QPixmap *outPixmap)
{
    if (!outPixmap) {
        return false;
    }

    const QList<QScreen *> screens = qApp->screens();
    QRect virtualRect;
    for (QScreen *screen : screens) {
        virtualRect = virtualRect.united(screen->geometry());
    }

    QPixmap fullDesktop(virtualRect.size());
    fullDesktop.fill(Qt::black);
    QPainter painter(&fullDesktop);
    for (QScreen *screen : screens) {
        QPixmap pm = grabScreenRect(screen->geometry());
        painter.drawPixmap(screen->geometry().topLeft() - virtualRect.topLeft(), pm);
    }
    painter.end();

    ScreenCaptureWidget *widget = new ScreenCaptureWidget(fullDesktop);
    widget->show();
    widget->activateWindow();
    widget->raise();

    QEventLoop loop;
    QObject::connect(widget, &QWidget::destroyed, &loop, &QEventLoop::quit);
    loop.exec();

    if (widget->wasConfirmed() && !widget->capturedRegion().isNull()) {
        *outPixmap = widget->capturedRegion();
        return true;
    }
    return false;
}
