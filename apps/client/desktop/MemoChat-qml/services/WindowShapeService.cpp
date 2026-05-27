#include "WindowShapeService.h"

#include <QPainterPath>
#include <QPointer>
#include <QRegion>
#include <QWindow>
#include <algorithm>

namespace {

void applyRoundedMask(QWindow *window, int radius)
{
    if (!window) {
        return;
    }

    const int width = window->width();
    const int height = window->height();
    if (width <= 0 || height <= 0) {
        return;
    }

    const int max_radius = std::min(width, height) / 2;
    const int safe_radius = std::max(0, std::min(radius, max_radius));

    if (safe_radius == 0) {
        window->setMask(QRegion(0, 0, width, height));
        return;
    }

    QPainterPath path;
    path.addRoundedRect(QRectF(0.0, 0.0, width, height), safe_radius, safe_radius);
    window->setMask(QRegion(path.toFillPolygon().toPolygon()));
}

} // namespace

void WindowShapeService::bindRoundedMask(QWindow *window, int radius)
{
    if (!window) {
        return;
    }

    const int applied_radius = std::max(0, radius);
    const QPointer<QWindow> tracked_window(window);

    auto refresh_mask = [tracked_window, applied_radius]() {
        if (!tracked_window) {
            return;
        }
        applyRoundedMask(tracked_window.data(), applied_radius);
    };

    QObject::connect(window, &QWindow::widthChanged, window, [refresh_mask](int) {
        refresh_mask();
    });
    QObject::connect(window, &QWindow::heightChanged, window, [refresh_mask](int) {
        refresh_mask();
    });
    QObject::connect(window, &QWindow::visibleChanged, window, [refresh_mask](bool) {
        refresh_mask();
    });

    refresh_mask();
}
