#include "ScreenCaptureWidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QScreen>
#include <QCursor>
#include <QDebug>

ScreenCaptureWidget::ScreenCaptureWidget(const QPixmap &desktopPixmap, QWidget *parent)
    : QWidget(parent)
    , _desktopPixmap(desktopPixmap)
    , _devicePixelRatio(qApp->devicePixelRatio())
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
                   | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);

    // Cover the full virtual screen
    QRect screenGeo = qApp->primaryScreen()->geometry();
    for (QScreen *scr : qApp->screens()) {
        screenGeo = screenGeo.united(scr->geometry());
    }
    setGeometry(screenGeo);
}

ScreenCaptureWidget::~ScreenCaptureWidget() = default;

void ScreenCaptureWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Draw the full desktop screenshot as background
    painter.drawPixmap(rect(), _desktopPixmap, _desktopPixmap.rect());

    // Draw semi-transparent overlay outside selection
    if (_selecting && _selectionRect.isValid()) {
        QRegion mask(rect());
        mask = mask.subtracted(_selectionRect.normalized());
        painter.setClipRegion(mask);
        painter.fillRect(rect(), QColor(0, 0, 0, 100));

        // Draw selection border
        painter.setClipping(false);
        painter.setPen(QPen(Qt::white, 1.5));
        painter.drawRect(_selectionRect.normalized().adjusted(0, 0, -1, -1));

        // Draw corner handles
        const int handleSize = 6;
        QList<QRect> handles = {
            QRect(_selectionRect.topLeft().x()     - handleSize/2, _selectionRect.topLeft().y()     - handleSize/2, handleSize, handleSize),
            QRect(_selectionRect.topRight().x()    - handleSize/2, _selectionRect.topRight().y()    - handleSize/2, handleSize, handleSize),
            QRect(_selectionRect.bottomLeft().x()   - handleSize/2, _selectionRect.bottomLeft().y()   - handleSize/2, handleSize, handleSize),
            QRect(_selectionRect.bottomRight().x() - handleSize/2, _selectionRect.bottomRight().y()  - handleSize/2, handleSize, handleSize),
        };
        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);
        for (const QRect &h : handles) {
            painter.drawRect(h);
        }

        // Draw size hint
        if (_selectionRect.width() > 60 && _selectionRect.height() > 30) {
            const QString sizeText = QString::number(_selectionRect.width())
                                       + " x " + QString::number(_selectionRect.height());
            QRect textBg = QRect(_selectionRect.center().x() - 40,
                                  _selectionRect.bottom() + 4,
                                  80, 18);
            painter.fillRect(textBg, QColor(0, 0, 0, 160));
            painter.setPen(Qt::white);
            QFont f = painter.font();
            f.setPixelSize(11);
            painter.setFont(f);
            painter.drawText(textBg, Qt::AlignHCenter | Qt::AlignVCenter, sizeText);
        }
    }

    // Draw instructions at top
    painter.setClipping(false);
    QRect instrBg(geometry().center().x() - 140, 20, 280, 26);
    painter.fillRect(instrBg, QColor(0, 0, 0, 160));
    painter.setPen(Qt::white);
    QFont f = painter.font();
    f.setPixelSize(12);
    painter.setFont(f);
    painter.drawText(instrBg, Qt::AlignHCenter | Qt::AlignVCenter,
                     QStringLiteral("拖拽选择区域  |  Enter确认  |  Esc取消"));
}

void ScreenCaptureWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton || event->button() == Qt::MiddleButton) {
        cancelSelection();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        _selecting = true;
        _startPos = event->pos();
        _currentPos = event->pos();
        updateSelectionRect();
        update();
    }
}

void ScreenCaptureWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!_selecting) return;
    _currentPos = event->pos();
    updateSelectionRect();
    update();
}

void ScreenCaptureWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && _selecting) {
        _selecting = false;
        _currentPos = event->pos();
        updateSelectionRect();
        if (_selectionRect.width() > 4 && _selectionRect.height() > 4) {
            finishSelection();
        }
        update();
    }
}

void ScreenCaptureWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        cancelSelection();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (_selectionRect.isValid() && _selectionRect.width() > 4 && _selectionRect.height() > 4) {
            finishSelection();
        }
    } else if (event->key() == Qt::Key_Space) {
        cancelSelection();
    }
}

void ScreenCaptureWidget::wheelEvent(QWheelEvent *)
{
    // Prevent scroll from propagating
}

void ScreenCaptureWidget::updateSelectionRect()
{
    const QPoint topLeft(std::min(_startPos.x(), _currentPos.x()),
                          std::min(_startPos.y(), _currentPos.y()));
    const QPoint bottomRight(std::max(_startPos.x(), _currentPos.x()),
                              std::max(_startPos.y(), _currentPos.y()));
    _selectionRect = QRect(topLeft, bottomRight);
    _selectionRect = _selectionRect.normalized();
}

void ScreenCaptureWidget::finishSelection()
{
    if (!_selectionRect.isValid() || _selectionRect.width() < 4 || _selectionRect.height() < 4) {
        return;
    }
    // Capture from original desktop pixmap (which may have devicePixelRatio scale)
    _capturedPixmap = _desktopPixmap.copy(_selectionRect);
    _confirmed = true;
    close();
}

void ScreenCaptureWidget::cancelSelection()
{
    _capturedPixmap = QPixmap();
    _confirmed = false;
    close();
}
