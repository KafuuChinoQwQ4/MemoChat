#include "Live2DAvatarOpenGLRenderer.h"

#include "Live2DOfficialOpenGLRenderer.h"

#include <QDebug>
#include <QGuiApplication>
#include <QMutex>
#include <QMutexLocker>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QRect>
#include <QSurfaceFormat>
#include <QtMath>

namespace
{
constexpr int kRenderEdge = 1536;
constexpr int kAvatarEdge = 512;
constexpr int kAlphaThreshold = 8;

QRect alphaBoundsInRect(const QImage& image, const QRect& scanRect, int alphaThreshold = kAlphaThreshold)
{
    if (image.isNull())
    {
        return {};
    }

    const QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    const QRect rect = scanRect.intersected(argb.rect());
    if (rect.isEmpty())
    {
        return {};
    }

    int left = rect.right() + 1;
    int top = rect.bottom() + 1;
    int right = rect.left() - 1;
    int bottom = rect.top() - 1;

    for (int y = rect.top(); y <= rect.bottom(); ++y)
    {
        const QRgb* line = reinterpret_cast<const QRgb*>(argb.constScanLine(y));
        for (int x = rect.left(); x <= rect.right(); ++x)
        {
            if (qAlpha(line[x]) <= alphaThreshold)
            {
                continue;
            }
            left = qMin(left, x);
            top = qMin(top, y);
            right = qMax(right, x);
            bottom = qMax(bottom, y);
        }
    }

    if (right < left || bottom < top)
    {
        return {};
    }
    return QRect(QPoint(left, top), QPoint(right, bottom));
}

QRect alphaBounds(const QImage& image, int alphaThreshold = kAlphaThreshold)
{
    return alphaBoundsInRect(image, image.rect(), alphaThreshold);
}

QPointF alphaCentroid(const QImage& image, const QRect& scanRect, int alphaThreshold = kAlphaThreshold)
{
    if (image.isNull())
    {
        return {};
    }

    const QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    const QRect rect = scanRect.intersected(argb.rect());
    if (rect.isEmpty())
    {
        return {};
    }

    double weightedX = 0.0;
    double weightedY = 0.0;
    double weightSum = 0.0;

    for (int y = rect.top(); y <= rect.bottom(); ++y)
    {
        const QRgb* line = reinterpret_cast<const QRgb*>(argb.constScanLine(y));
        for (int x = rect.left(); x <= rect.right(); ++x)
        {
            const int alpha = qAlpha(line[x]);
            if (alpha <= alphaThreshold)
            {
                continue;
            }
            weightedX += static_cast<double>(x) * alpha;
            weightedY += static_cast<double>(y) * alpha;
            weightSum += alpha;
        }
    }

    if (weightSum <= 0.0)
    {
        return {};
    }
    return QPointF(weightedX / weightSum, weightedY / weightSum);
}

QRect clampedSquareAround(const QPointF& center, int side, const QRect& imageRect)
{
    if (side <= 0 || imageRect.isEmpty())
    {
        return {};
    }

    side = qMin(side, qMin(imageRect.width(), imageRect.height()));
    int x = qRound(center.x() - side / 2.0);
    int y = qRound(center.y() - side / 2.0);
    x = qBound(imageRect.left(), x, imageRect.right() - side + 1);
    y = qBound(imageRect.top(), y, imageRect.bottom() - side + 1);
    return QRect(x, y, side, side).intersected(imageRect);
}

QRect avatarCropRect(const QImage& source)
{
    const QRect model = alphaBounds(source);
    if (model.isEmpty())
    {
        return {};
    }

    const QRect upperScan(model.left(), model.top(), model.width(), qMax(1, qRound(model.height() * 0.40)));
    QRect upper = alphaBoundsInRect(source, upperScan);
    if (upper.isEmpty())
    {
        upper = model;
    }

    const QRect faceScan(model.left(),
                         model.top() + qRound(model.height() * 0.06),
                         model.width(),
                         qMax(1, qRound(model.height() * 0.26)));
    QRect face = alphaBoundsInRect(source, faceScan);
    if (face.isEmpty())
    {
        face = upper;
    }

    QPointF center = alphaCentroid(source, face);
    if (center.isNull())
    {
        center = face.center();
    }

    const int sideFromUpper = qRound(qMax(upper.width() * 0.92, upper.height() * 0.70));
    const int sideFromFace = qRound(qMax(face.width() * 1.10, face.height() * 1.02));
    const int sideFromModel = qRound(qMax(model.width() * 0.50, model.height() * 0.24));
    int side = qMax(96, qMax(sideFromModel, qMin(qMax(sideFromUpper, sideFromFace), qRound(model.height() * 0.36))));
    side = qMin(side, qMin(source.width(), source.height()));

    const qreal portraitCenterY = model.top() + model.height() * 0.23;
    center.setY(center.y() * 0.42 + portraitCenterY * 0.58 - side * 0.18);
    return clampedSquareAround(center, side, source.rect());
}

QSurfaceFormat avatarSurfaceFormat()
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setAlphaBufferSize(8);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    return format;
}

void setError(QString* error, const QString& message)
{
    if (error)
    {
        *error = message;
    }
}
} // namespace

QString Live2DAvatarOpenGLRenderer::rendererName()
{
    return QStringLiteral("cubism-official-opengl-avatar");
}

QImage Live2DAvatarOpenGLRenderer::renderAvatar(const QString& modelJsonPath, QString* error) const
{
    static QMutex avatarRenderMutex;
    QMutexLocker locker(&avatarRenderMutex);

    if (!QGuiApplication::instance())
    {
        setError(error, QStringLiteral("QGuiApplication is not available for Live2D avatar rendering"));
        return {};
    }

    QOffscreenSurface surface;
    surface.setFormat(avatarSurfaceFormat());
    surface.create();
    if (!surface.isValid())
    {
        setError(error, QStringLiteral("failed to create offscreen surface for Live2D avatar"));
        return {};
    }

    QOpenGLContext context;
    context.setFormat(surface.format());
    if (QOpenGLContext* shareContext = QOpenGLContext::globalShareContext())
    {
        context.setShareContext(shareContext);
    }
    if (!context.create())
    {
        setError(error, QStringLiteral("failed to create OpenGL context for Live2D avatar"));
        return {};
    }
    if (!context.makeCurrent(&surface))
    {
        setError(error, QStringLiteral("failed to activate OpenGL context for Live2D avatar"));
        return {};
    }

    QImage avatar;
    QString renderError;
    {
        QOpenGLFramebufferObjectFormat fboFormat;
        fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

        const QSize renderSize(kRenderEdge, kRenderEdge);
        QOpenGLFramebufferObject fbo(renderSize, fboFormat);
        if (!fbo.isValid())
        {
            renderError = QStringLiteral("failed to create Live2D avatar framebuffer");
        }
        else if (!fbo.bind())
        {
            renderError = QStringLiteral("failed to bind Live2D avatar framebuffer");
        }
        else
        {
            Live2DOfficialOpenGLRenderer renderer(modelJsonPath);
            if (!renderer.isReady())
            {
                renderError = renderer.errorString();
            }
            else
            {
                Live2DVisualState state;
                state.expression = QStringLiteral("neutral");
                state.motion = QStringLiteral("idle");
                state.emotion = QStringLiteral("neutral");
                state.gazeX = 0.5;
                state.gazeY = 0.48;
                state.idlePhase = 0.0;

                if (!renderer.render(renderSize, state))
                {
                    renderError = QStringLiteral("official OpenGL renderer returned false for Live2D avatar");
                }
                else
                {
                    if (QOpenGLFunctions* functions = context.functions())
                    {
                        functions->glFlush();
                    }
                    const QImage rendered = fbo.toImage(true).convertToFormat(QImage::Format_ARGB32_Premultiplied);
                    avatar = cropAvatarFrame(rendered, QSize(kAvatarEdge, kAvatarEdge));
                }
            }
            fbo.release();
        }
    }

    context.doneCurrent();

    if (avatar.isNull())
    {
        setError(error,
                 renderError.isEmpty()
                 ? QStringLiteral("Live2D avatar crop produced an empty image")
                 : renderError);
    }
    else
    {
        setError(error, QString());
    }
    return avatar;
}

QImage Live2DAvatarOpenGLRenderer::cropAvatarFrame(const QImage& source, const QSize& outputSize)
{
    if (source.isNull() || outputSize.isEmpty())
    {
        return {};
    }

    const QRect crop = avatarCropRect(source);
    if (crop.isEmpty())
    {
        return {};
    }

    QImage avatar(outputSize, QImage::Format_ARGB32_Premultiplied);
    avatar.fill(Qt::transparent);

    QPainter painter(&avatar);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(QRectF(QPointF(0, 0), QSizeF(outputSize)), source, crop);
    return avatar;
}
