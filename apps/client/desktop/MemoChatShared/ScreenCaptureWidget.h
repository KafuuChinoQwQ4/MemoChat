#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QMenu>

class ScreenCaptureWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScreenCaptureWidget(const QPixmap &desktopPixmap, QWidget *parent = nullptr);
    ~ScreenCaptureWidget();

    QPixmap capturedRegion() const { return _capturedPixmap; }
    bool wasConfirmed() const { return _confirmed; }

signals:
    void regionSelected(const QPixmap &pixmap);
    void cancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QPixmap _desktopPixmap;
    QPixmap _scaledPixmap;
    QRect _selectionRect;
    QPoint _startPos;
    QPoint _currentPos;
    bool _selecting = false;
    bool _confirmed = false;
    QPixmap _capturedPixmap;
    qreal _devicePixelRatio = 1.0;

    void updateSelectionRect();
    void finishSelection();
    void cancelSelection();
    void drawCrosshair(QPainter *painter, const QPoint &pos);
};
