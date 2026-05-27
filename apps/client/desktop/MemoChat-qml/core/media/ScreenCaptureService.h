#pragma once

#include <QObject>
#include <QString>
#include <QPixmap>

class ScreenCaptureService : public QObject {
    Q_OBJECT
public:
    explicit ScreenCaptureService(QObject *parent = nullptr);
    ~ScreenCaptureService();

    static bool captureScreen(QPixmap *outPixmap);

signals:
    void captureCompleted(const QPixmap &pixmap);
    void captureCancelled();
};
