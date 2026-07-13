#include "Live2DCoreRenderer.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace
{
struct CsmVector2
{
    float X;
    float Y;
};

struct CsmVector4
{
    float X;
    float Y;
    float Z;
    float W;
};

constexpr size_t MocAlignment = 64;
constexpr size_t ModelAlignment = 16;
constexpr unsigned char DrawableFlagIsVisible = 1 << 0;
constexpr unsigned char DrawableFlagIsInvertedMask = 1 << 3;
constexpr int CubismBlendModeNormal = 0;
constexpr int CubismBlendModeAdditive = 1;
constexpr int CubismBlendModeMultiplicative = 2;

struct ColorTransform
{
    qreal multiplyRed = 1.0;
    qreal multiplyGreen = 1.0;
    qreal multiplyBlue = 1.0;
    qreal screenRed = 0.0;
    qreal screenGreen = 0.0;
    qreal screenBlue = 0.0;
};

struct StraightColor
{
    qreal red = 0.0;
    qreal green = 0.0;
    qreal blue = 0.0;
    qreal alpha = 0.0;
};

QString clientSourcePath(const QString& relativePath)
{
#ifdef MEMOCHAT_QML_SOURCE_DIR
    const QString root = QString::fromUtf8(MEMOCHAT_QML_SOURCE_DIR);
#else
    const QString root = QCoreApplication::applicationDirPath();
#endif
    return QDir(root).absoluteFilePath(relativePath);
}

QStringList coreLibraryCandidates()
{
    QStringList candidates;

    const QString envRoot = qEnvironmentVariable("MEMOCHAT_LIVE2D_SDK_ROOT");
    if (!envRoot.isEmpty())
    {
        candidates << QDir(envRoot).absoluteFilePath(QStringLiteral("Core/dll/linux/x86_64/libLive2DCubismCore.so"));
        candidates << QDir(envRoot).absoluteFilePath(QStringLiteral("Core/lib/linux/x86_64/libLive2DCubismCore.so"));
    }

#ifdef MEMOCHAT_LIVE2D_SDK_ROOT
    const QString cmakeRoot = QString::fromUtf8(MEMOCHAT_LIVE2D_SDK_ROOT);
    if (!cmakeRoot.isEmpty())
    {
        candidates << QDir(cmakeRoot).absoluteFilePath(QStringLiteral("Core/dll/linux/x86_64/libLive2DCubismCore.so"));
        candidates << QDir(cmakeRoot).absoluteFilePath(QStringLiteral("Core/lib/linux/x86_64/libLive2DCubismCore.so"));
    }
#endif

    candidates << QStringLiteral("/data/third_party/live2d/CubismSdkForNative-current/Core/dll/linux/x86_64/libLive2DCubismCore.so");
    candidates << QStringLiteral("/data/third_party/live2d/CubismSdkForNative-5-r.5/Core/dll/linux/x86_64/libLive2DCubismCore.so");
    candidates << QStringLiteral("Live2DCubismCore");
    candidates << QStringLiteral("libLive2DCubismCore.so");
    return candidates;
}

QByteArray readFileBytes(const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (error)
        {
            *error = QStringLiteral("failed to open %1: %2").arg(path, file.errorString());
        }
        return {};
    }
    return file.readAll();
}

template <typename T> T resolve(QLibrary& library, const char* symbol)
{
    return reinterpret_cast<T>(library.resolve(symbol));
}

float clamped(float value, float minimum, float maximum)
{
    return std::max(minimum, std::min(maximum, value));
}

float blended(float current, float value, float weight)
{
    return current * (1.0f - weight) + value * weight;
}

qreal edgeValue(const QPointF& a, const QPointF& b, const QPointF& point)
{
    return (point.x() - a.x()) * (b.y() - a.y()) - (point.y() - a.y()) * (b.x() - a.x());
}

int colorByte(qreal value)
{
    return qBound(0, static_cast<int>(value * 255.0 + 0.5), 255);
}

StraightColor straightColorFromPixel(QRgb pixel)
{
    return {qRed(pixel) / 255.0, qGreen(pixel) / 255.0, qBlue(pixel) / 255.0, qAlpha(pixel) / 255.0};
}

QRgb straightColorToPixel(const StraightColor& color)
{
    return qRgba(colorByte(color.red), colorByte(color.green), colorByte(color.blue), colorByte(color.alpha));
}

StraightColor transformedSourceColor(const QColor& sample, qreal opacity, const ColorTransform& transform)
{
    StraightColor source;
    source.red = sample.redF();
    source.green = sample.greenF();
    source.blue = sample.blueF();
    source.alpha = sample.alphaF() * opacity;

    source.red = qBound<qreal>(0.0,
                               source.red * transform.multiplyRed + transform.screenRed -
                                   (source.red * transform.multiplyRed * transform.screenRed),
                               1.0);
    source.green = qBound<qreal>(0.0,
                                 source.green * transform.multiplyGreen + transform.screenGreen -
                                     (source.green * transform.multiplyGreen * transform.screenGreen),
                                 1.0);
    source.blue = qBound<qreal>(0.0,
                                source.blue * transform.multiplyBlue + transform.screenBlue -
                                    (source.blue * transform.multiplyBlue * transform.screenBlue),
                                1.0);
    source.alpha = qBound<qreal>(0.0, source.alpha, 1.0);
    return source;
}

StraightColor blendSourceOver(const StraightColor& source, const StraightColor& destination)
{
    const qreal inverseSourceAlpha = 1.0 - source.alpha;
    StraightColor result;
    result.alpha = source.alpha + destination.alpha * inverseSourceAlpha;
    if (result.alpha <= 0.0)
    {
        return {};
    }

    result.red = (source.red * source.alpha + destination.red * destination.alpha * inverseSourceAlpha) / result.alpha;
    result.green =
        (source.green * source.alpha + destination.green * destination.alpha * inverseSourceAlpha) / result.alpha;
    result.blue =
        (source.blue * source.alpha + destination.blue * destination.alpha * inverseSourceAlpha) / result.alpha;
    return result;
}

StraightColor blendAdditive(const StraightColor& source, const StraightColor& destination)
{
    StraightColor result;
    result.alpha = qBound<qreal>(0.0, destination.alpha + source.alpha, 1.0);
    if (result.alpha <= 0.0)
    {
        return {};
    }

    const qreal red = destination.red * destination.alpha + source.red * source.alpha;
    const qreal green = destination.green * destination.alpha + source.green * source.alpha;
    const qreal blue = destination.blue * destination.alpha + source.blue * source.alpha;
    result.red = qBound<qreal>(0.0, red / result.alpha, 1.0);
    result.green = qBound<qreal>(0.0, green / result.alpha, 1.0);
    result.blue = qBound<qreal>(0.0, blue / result.alpha, 1.0);
    return result;
}

StraightColor blendMultiplicative(const StraightColor& source, const StraightColor& destination)
{
    if (destination.alpha <= 0.0)
    {
        return {};
    }

    const qreal inverseSourceAlpha = 1.0 - source.alpha;
    StraightColor result;
    result.alpha = destination.alpha;
    result.red =
        qBound<qreal>(0.0, source.red * source.alpha * destination.red + destination.red * inverseSourceAlpha, 1.0);
    result.green =
        qBound<qreal>(0.0,
                      source.green * source.alpha * destination.green + destination.green * inverseSourceAlpha,
                      1.0);
    result.blue =
        qBound<qreal>(0.0, source.blue * source.alpha * destination.blue + destination.blue * inverseSourceAlpha, 1.0);
    return result;
}

QColor sampleTexturePixel(const QImage& texture, qreal textureX, qreal textureY)
{
    if (texture.isNull())
    {
        return {};
    }

    textureX = qBound<qreal>(0.0, textureX, texture.width() - 1.0);
    textureY = qBound<qreal>(0.0, textureY, texture.height() - 1.0);
    const int x0 = static_cast<int>(std::floor(textureX));
    const int y0 = static_cast<int>(std::floor(textureY));
    const int x1 = qMin(x0 + 1, texture.width() - 1);
    const int y1 = qMin(y0 + 1, texture.height() - 1);
    const qreal fx = textureX - x0;
    const qreal fy = textureY - y0;

    const QRgb p00 = texture.pixel(x0, y0);
    const QRgb p10 = texture.pixel(x1, y0);
    const QRgb p01 = texture.pixel(x0, y1);
    const QRgb p11 = texture.pixel(x1, y1);

    const auto interpolate = [fx, fy](int c00, int c10, int c01, int c11)
    {
        const qreal top = c00 * (1.0 - fx) + c10 * fx;
        const qreal bottom = c01 * (1.0 - fx) + c11 * fx;
        return static_cast<int>(top * (1.0 - fy) + bottom * fy + 0.5);
    };

    const int alpha = interpolate(qAlpha(p00), qAlpha(p10), qAlpha(p01), qAlpha(p11));
    if (alpha <= 0)
    {
        return {};
    }
    return QColor(interpolate(qRed(p00), qRed(p10), qRed(p01), qRed(p11)),
                  interpolate(qGreen(p00), qGreen(p10), qGreen(p01), qGreen(p11)),
                  interpolate(qBlue(p00), qBlue(p10), qBlue(p01), qBlue(p11)),
                  alpha);
}

qreal maskOpacityAt(const QImage* clipMask, int x, int y, bool invertedMask)
{
    if (!clipMask || clipMask->isNull())
    {
        return 1.0;
    }

    const uchar* line = clipMask->constScanLine(y);
    const qreal alpha = line[x] / 255.0;
    return invertedMask ? (1.0 - alpha) : alpha;
}

void blendPixel(QImage& target,
                int x,
                int y,
                const QColor& sample,
                qreal opacity,
                int blendMode,
                const ColorTransform& transform,
                const QImage* clipMask,
                bool invertedMask)
{
    const qreal maskedOpacity = opacity * maskOpacityAt(clipMask, x, y, invertedMask);
    if (maskedOpacity <= 0.0)
    {
        return;
    }

    const StraightColor source = transformedSourceColor(sample, maskedOpacity, transform);
    if (source.alpha <= 0.0)
    {
        return;
    }

    QRgb* line = reinterpret_cast<QRgb*>(target.scanLine(y));
    const StraightColor destination = straightColorFromPixel(line[x]);

    StraightColor result;
    switch (blendMode)
    {
        case CubismBlendModeAdditive:
            result = blendAdditive(source, destination);
            break;
        case CubismBlendModeMultiplicative:
            result = blendMultiplicative(source, destination);
            break;
        case CubismBlendModeNormal:
        default:
            result = blendSourceOver(source, destination);
            break;
    }

    line[x] = straightColorToPixel(result);
}

void paintTexturedTriangle(QImage& target,
                           const QImage& texture,
                           const QPointF& p0,
                           const QPointF& p1,
                           const QPointF& p2,
                           const QPointF& t0,
                           const QPointF& t1,
                           const QPointF& t2,
                           qreal opacity,
                           int blendMode,
                           const ColorTransform& transform,
                           const QImage* clipMask,
                           bool invertedMask)
{
    const qreal area = edgeValue(p0, p1, p2);
    if (qFuzzyIsNull(area))
    {
        return;
    }

    QRect bounds = QRectF(p0, QSizeF(0, 0)).toAlignedRect();
    bounds |= QRectF(p1, QSizeF(0, 0)).toAlignedRect();
    bounds |= QRectF(p2, QSizeF(0, 0)).toAlignedRect();
    bounds = bounds.adjusted(-1, -1, 1, 1).intersected(target.rect());
    if (bounds.isEmpty())
    {
        return;
    }

    for (int y = bounds.top(); y <= bounds.bottom(); ++y)
    {
        for (int x = bounds.left(); x <= bounds.right(); ++x)
        {
            const QPointF samplePoint(x + 0.5, y + 0.5);
            const qreal w0 = edgeValue(p1, p2, samplePoint) / area;
            const qreal w1 = edgeValue(p2, p0, samplePoint) / area;
            const qreal w2 = edgeValue(p0, p1, samplePoint) / area;
            if (w0 < -0.001 || w1 < -0.001 || w2 < -0.001)
            {
                continue;
            }

            const qreal textureX = t0.x() * w0 + t1.x() * w1 + t2.x() * w2;
            const qreal textureY = t0.y() * w0 + t1.y() * w1 + t2.y() * w2;
            const QColor color = sampleTexturePixel(texture, textureX, textureY);
            blendPixel(target, x, y, color, opacity, blendMode, transform, clipMask, invertedMask);
        }
    }
}

void paintMaskTriangle(QImage& target,
                       const QImage& texture,
                       const QPointF& p0,
                       const QPointF& p1,
                       const QPointF& p2,
                       const QPointF& t0,
                       const QPointF& t1,
                       const QPointF& t2,
                       qreal opacity)
{
    const qreal area = edgeValue(p0, p1, p2);
    if (qFuzzyIsNull(area))
    {
        return;
    }

    QRect bounds = QRectF(p0, QSizeF(0, 0)).toAlignedRect();
    bounds |= QRectF(p1, QSizeF(0, 0)).toAlignedRect();
    bounds |= QRectF(p2, QSizeF(0, 0)).toAlignedRect();
    bounds = bounds.adjusted(-1, -1, 1, 1).intersected(target.rect());
    if (bounds.isEmpty())
    {
        return;
    }

    for (int y = bounds.top(); y <= bounds.bottom(); ++y)
    {
        uchar* line = target.scanLine(y);
        for (int x = bounds.left(); x <= bounds.right(); ++x)
        {
            const QPointF samplePoint(x + 0.5, y + 0.5);
            const qreal w0 = edgeValue(p1, p2, samplePoint) / area;
            const qreal w1 = edgeValue(p2, p0, samplePoint) / area;
            const qreal w2 = edgeValue(p0, p1, samplePoint) / area;
            if (w0 < -0.001 || w1 < -0.001 || w2 < -0.001)
            {
                continue;
            }

            const qreal textureX = t0.x() * w0 + t1.x() * w1 + t2.x() * w2;
            const qreal textureY = t0.y() * w0 + t1.y() * w1 + t2.y() * w2;
            const QColor color = sampleTexturePixel(texture, textureX, textureY);
            const int sourceAlpha = qBound(0, static_cast<int>(color.alphaF() * opacity * 255.0 + 0.5), 255);
            if (sourceAlpha <= 0)
            {
                continue;
            }
            const int currentAlpha = line[x];
            line[x] = static_cast<uchar>(sourceAlpha + currentAlpha * (255 - sourceAlpha) / 255);
        }
    }
}
} // namespace

struct Live2DCoreRenderer::AlignedBuffer
{
    AlignedBuffer() = default;

    AlignedBuffer(const QByteArray& bytes, size_t alignment)
        : size(bytes.size())
    {
        if (size <= 0)
        {
            return;
        }
        const size_t padded = static_cast<size_t>(size);
        void* raw = nullptr;
#if defined(_MSC_VER)
        raw = _aligned_malloc(padded, alignment);
        if (!raw)
        {
            return;
        }
#else
        if (posix_memalign(&raw, alignment, padded) != 0)
        {
            raw = nullptr;
            return;
        }
#endif
        data = raw;
        std::memcpy(data, bytes.constData(), static_cast<size_t>(size));
    }

    ~AlignedBuffer()
    {
#if defined(_MSC_VER)
        _aligned_free(data);
#else
        std::free(data);
#endif
    }

    AlignedBuffer(const AlignedBuffer&) = delete;
    AlignedBuffer& operator=(const AlignedBuffer&) = delete;

    void* data = nullptr;
    qsizetype size = 0;
};

struct Live2DCoreRenderer::CoreApi
{
    using ReviveMocInPlace = csmMoc* (*) (void*, unsigned int);
    using GetSizeofModel = unsigned int (*)(const csmMoc*);
    using InitializeModelInPlace = csmModel* (*) (const csmMoc*, void*, unsigned int);
    using UpdateModel = void (*)(csmModel*);
    using ReadCanvasInfo = void (*)(const csmModel*, CsmVector2*, CsmVector2*, float*);
    using GetParameterCount = int (*)(const csmModel*);
    using GetParameterIds = const char** (*) (const csmModel*);
    using GetParameterMinimumValues = const float* (*) (const csmModel*);
    using GetParameterMaximumValues = const float* (*) (const csmModel*);
    using GetParameterDefaultValues = const float* (*) (const csmModel*);
    using GetParameterValues = float* (*) (csmModel*);
    using GetDrawableCount = int (*)(const csmModel*);
    using GetRenderOrders = const int* (*) (const csmModel*);
    using GetDrawableConstantFlags = const unsigned char* (*) (const csmModel*);
    using GetDrawableDynamicFlags = const unsigned char* (*) (const csmModel*);
    using GetDrawableBlendModes = const int* (*) (const csmModel*);
    using GetDrawableMaskCounts = const int* (*) (const csmModel*);
    using GetDrawableMasks = const int** (*) (const csmModel*);
    using GetDrawableTextureIndices = const int* (*) (const csmModel*);
    using GetDrawableOpacities = const float* (*) (const csmModel*);
    using GetDrawableVertexCounts = const int* (*) (const csmModel*);
    using GetDrawableVertexPositions = const CsmVector2** (*) (const csmModel*);
    using GetDrawableVertexUvs = const CsmVector2** (*) (const csmModel*);
    using GetDrawableIndexCounts = const int* (*) (const csmModel*);
    using GetDrawableIndices = const unsigned short** (*) (const csmModel*);
    using GetDrawableMultiplyColors = const CsmVector4* (*) (const csmModel*);
    using GetDrawableScreenColors = const CsmVector4* (*) (const csmModel*);

    bool load()
    {
        if (attempted)
        {
            return loaded;
        }
        attempted = true;

        for (const QString& candidate : coreLibraryCandidates())
        {
            if (candidate.contains(QLatin1Char('/')) && !QFileInfo::exists(candidate))
            {
                continue;
            }
            library.setFileName(candidate);
            if (!library.load())
            {
                continue;
            }

            reviveMocInPlace = resolve<ReviveMocInPlace>(library, "csmReviveMocInPlace");
            getSizeofModel = resolve<GetSizeofModel>(library, "csmGetSizeofModel");
            initializeModelInPlace = resolve<InitializeModelInPlace>(library, "csmInitializeModelInPlace");
            updateModel = resolve<UpdateModel>(library, "csmUpdateModel");
            readCanvasInfo = resolve<ReadCanvasInfo>(library, "csmReadCanvasInfo");
            getParameterCount = resolve<GetParameterCount>(library, "csmGetParameterCount");
            getParameterIds = resolve<GetParameterIds>(library, "csmGetParameterIds");
            getParameterMinimumValues = resolve<GetParameterMinimumValues>(library, "csmGetParameterMinimumValues");
            getParameterMaximumValues = resolve<GetParameterMaximumValues>(library, "csmGetParameterMaximumValues");
            getParameterDefaultValues = resolve<GetParameterDefaultValues>(library, "csmGetParameterDefaultValues");
            getParameterValues = resolve<GetParameterValues>(library, "csmGetParameterValues");
            getDrawableCount = resolve<GetDrawableCount>(library, "csmGetDrawableCount");
            getRenderOrders = resolve<GetRenderOrders>(library, "csmGetRenderOrders");
            getDrawableConstantFlags = resolve<GetDrawableConstantFlags>(library, "csmGetDrawableConstantFlags");
            getDrawableDynamicFlags = resolve<GetDrawableDynamicFlags>(library, "csmGetDrawableDynamicFlags");
            getDrawableBlendModes = resolve<GetDrawableBlendModes>(library, "csmGetDrawableBlendModes");
            getDrawableMaskCounts = resolve<GetDrawableMaskCounts>(library, "csmGetDrawableMaskCounts");
            getDrawableMasks = resolve<GetDrawableMasks>(library, "csmGetDrawableMasks");
            getDrawableTextureIndices = resolve<GetDrawableTextureIndices>(library, "csmGetDrawableTextureIndices");
            getDrawableOpacities = resolve<GetDrawableOpacities>(library, "csmGetDrawableOpacities");
            getDrawableVertexCounts = resolve<GetDrawableVertexCounts>(library, "csmGetDrawableVertexCounts");
            getDrawableVertexPositions = resolve<GetDrawableVertexPositions>(library, "csmGetDrawableVertexPositions");
            getDrawableVertexUvs = resolve<GetDrawableVertexUvs>(library, "csmGetDrawableVertexUvs");
            getDrawableIndexCounts = resolve<GetDrawableIndexCounts>(library, "csmGetDrawableIndexCounts");
            getDrawableIndices = resolve<GetDrawableIndices>(library, "csmGetDrawableIndices");
            getDrawableMultiplyColors = resolve<GetDrawableMultiplyColors>(library, "csmGetDrawableMultiplyColors");
            getDrawableScreenColors = resolve<GetDrawableScreenColors>(library, "csmGetDrawableScreenColors");

            loaded = reviveMocInPlace && getSizeofModel && initializeModelInPlace && updateModel && readCanvasInfo &&
                     getParameterCount && getParameterIds && getParameterMinimumValues && getParameterMaximumValues &&
                     getParameterDefaultValues && getParameterValues && getDrawableCount && getRenderOrders &&
                     getDrawableConstantFlags && getDrawableDynamicFlags && getDrawableBlendModes &&
                     getDrawableMaskCounts && getDrawableMasks && getDrawableTextureIndices && getDrawableOpacities &&
                     getDrawableVertexCounts && getDrawableVertexPositions && getDrawableVertexUvs &&
                     getDrawableIndexCounts && getDrawableIndices;
            if (loaded)
            {
                loadedFrom = candidate;
                return true;
            }

            error = QStringLiteral("Cubism Core library %1 is missing required symbols").arg(candidate);
            library.unload();
        }

        if (error.isEmpty())
        {
            error = QStringLiteral("Live2D Cubism Core library was not found");
        }
        return false;
    }

    QLibrary library;
    bool attempted = false;
    bool loaded = false;
    QString loadedFrom;
    QString error;
    ReviveMocInPlace reviveMocInPlace = nullptr;
    GetSizeofModel getSizeofModel = nullptr;
    InitializeModelInPlace initializeModelInPlace = nullptr;
    UpdateModel updateModel = nullptr;
    ReadCanvasInfo readCanvasInfo = nullptr;
    GetParameterCount getParameterCount = nullptr;
    GetParameterIds getParameterIds = nullptr;
    GetParameterMinimumValues getParameterMinimumValues = nullptr;
    GetParameterMaximumValues getParameterMaximumValues = nullptr;
    GetParameterDefaultValues getParameterDefaultValues = nullptr;
    GetParameterValues getParameterValues = nullptr;
    GetDrawableCount getDrawableCount = nullptr;
    GetRenderOrders getRenderOrders = nullptr;
    GetDrawableConstantFlags getDrawableConstantFlags = nullptr;
    GetDrawableDynamicFlags getDrawableDynamicFlags = nullptr;
    GetDrawableBlendModes getDrawableBlendModes = nullptr;
    GetDrawableMaskCounts getDrawableMaskCounts = nullptr;
    GetDrawableMasks getDrawableMasks = nullptr;
    GetDrawableTextureIndices getDrawableTextureIndices = nullptr;
    GetDrawableOpacities getDrawableOpacities = nullptr;
    GetDrawableVertexCounts getDrawableVertexCounts = nullptr;
    GetDrawableVertexPositions getDrawableVertexPositions = nullptr;
    GetDrawableVertexUvs getDrawableVertexUvs = nullptr;
    GetDrawableIndexCounts getDrawableIndexCounts = nullptr;
    GetDrawableIndices getDrawableIndices = nullptr;
    GetDrawableMultiplyColors getDrawableMultiplyColors = nullptr;
    GetDrawableScreenColors getDrawableScreenColors = nullptr;
};

Live2DCoreRenderer::Live2DCoreRenderer(const QString& modelPath)
{
    if (modelPath.trimmed().isEmpty())
    {
        _error = QStringLiteral("Live2D model path is empty");
        return;
    }
    _ready = loadModelFile(modelPath);
}

Live2DCoreRenderer::~Live2DCoreRenderer() = default;

bool Live2DCoreRenderer::isReady() const
{
    return _ready;
}

QString Live2DCoreRenderer::errorString() const
{
    return _error;
}

QString Live2DCoreRenderer::rendererName() const
{
    return QStringLiteral("cubism-core");
}

bool Live2DCoreRenderer::isNative() const
{
    return true;
}

void Live2DCoreRenderer::paint(QPainter* painter, const QRectF& itemBounds, const Live2DVisualState& state)
{
    if (!painter || !_ready || !_model)
    {
        return;
    }

    const QRectF bounds = itemBounds.adjusted(8, 8, -8, -8);
    if (bounds.width() <= 0 || bounds.height() <= 0)
    {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    resetParameters();
    applyVisualState(state);
    coreApi().updateModel(_model);
    paintModel(painter, bounds.adjusted(4, 4, -4, -4));
}

QImage Live2DCoreRenderer::renderToImage(const QSize& size, const Live2DVisualState& state)
{
    if (size.isEmpty() || !_ready || !_model)
    {
        return {};
    }

    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    paint(&painter, QRectF(QPointF(0, 0), QSizeF(size)), state);
    return image;
}

Live2DCoreRenderer::CoreApi& Live2DCoreRenderer::coreApi()
{
    static CoreApi api;
    return api;
}

bool Live2DCoreRenderer::loadModelFile(const QString& modelPath)
{
    CoreApi& api = coreApi();
    if (!api.load())
    {
        _error = api.error;
        return false;
    }

    QString error;
    const QByteArray modelBytes = readFileBytes(modelPath, &error);
    if (modelBytes.isEmpty())
    {
        _error = error;
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(modelBytes);
    const QJsonObject root = doc.object();
    const QJsonObject fileReferences = root.value(QStringLiteral("FileReferences")).toObject();
    const QString mocName = fileReferences.value(QStringLiteral("Moc")).toString();
    const QJsonArray textures = fileReferences.value(QStringLiteral("Textures")).toArray();
    if (mocName.isEmpty() || textures.isEmpty())
    {
        _error = QStringLiteral("model3.json is missing Moc or Textures: %1").arg(modelPath);
        return false;
    }

    const QDir modelDir(QFileInfo(modelPath).absolutePath());
    QVector<QString> texturePaths;
    texturePaths.reserve(textures.size());
    for (const QJsonValue& texture : textures)
    {
        const QString texturePath = texture.toString();
        if (!texturePath.isEmpty())
        {
            texturePaths.push_back(texturePath);
        }
    }

    if (!loadMoc(modelDir.absoluteFilePath(mocName)))
    {
        return false;
    }
    if (!loadTextures(modelDir.absolutePath(), texturePaths))
    {
        return false;
    }

    initializeParameters();
    resetParameters();
    api.updateModel(_model);
    return true;
}

bool Live2DCoreRenderer::loadMoc(const QString& mocPath)
{
    QString error;
    const QByteArray mocBytes = readFileBytes(mocPath, &error);
    if (mocBytes.isEmpty())
    {
        _error = error;
        return false;
    }

    _mocBuffer = std::make_unique<AlignedBuffer>(mocBytes, MocAlignment);
    if (!_mocBuffer->data)
    {
        _error = QStringLiteral("failed to allocate aligned moc buffer");
        return false;
    }

    CoreApi& api = coreApi();
    _moc = api.reviveMocInPlace(_mocBuffer->data, static_cast<unsigned int>(_mocBuffer->size));
    if (!_moc)
    {
        _error = QStringLiteral("failed to revive moc: %1").arg(mocPath);
        return false;
    }

    const unsigned int modelSize = api.getSizeofModel(_moc);
    if (modelSize == 0)
    {
        _error = QStringLiteral("Cubism Core returned empty model size");
        return false;
    }
    _modelBuffer = std::make_unique<AlignedBuffer>(QByteArray(static_cast<qsizetype>(modelSize), '\0'), ModelAlignment);
    if (!_modelBuffer->data)
    {
        _error = QStringLiteral("failed to allocate aligned model buffer");
        return false;
    }
    _model = api.initializeModelInPlace(_moc, _modelBuffer->data, modelSize);
    if (!_model)
    {
        _error = QStringLiteral("failed to initialize Cubism model");
        return false;
    }
    return true;
}

bool Live2DCoreRenderer::loadTextures(const QString& modelDirectory, const QVector<QString>& texturePaths)
{
    _textures.clear();
    _textures.reserve(texturePaths.size());
    const QDir modelDir(modelDirectory);
    for (const QString& relativePath : texturePaths)
    {
        QImage image(modelDir.absoluteFilePath(relativePath));
        if (image.isNull())
        {
            _error = QStringLiteral("failed to load Live2D texture: %1").arg(modelDir.absoluteFilePath(relativePath));
            return false;
        }
        _textures.push_back(image.convertToFormat(QImage::Format_RGBA8888));
    }
    return true;
}

void Live2DCoreRenderer::initializeParameters()
{
    CoreApi& api = coreApi();
    const int count = api.getParameterCount(_model);
    if (count <= 0)
    {
        return;
    }
    const char** ids = api.getParameterIds(_model);
    const float* defaults = api.getParameterDefaultValues(_model);
    const float* minimums = api.getParameterMinimumValues(_model);
    const float* maximums = api.getParameterMaximumValues(_model);

    _parameterIndexById.clear();
    _parameterDefaults.resize(count);
    _parameterMinimums.resize(count);
    _parameterMaximums.resize(count);

    for (int i = 0; i < count; ++i)
    {
        _parameterIndexById.insert(QString::fromUtf8(ids[i]), i);
        _parameterDefaults[i] = defaults[i];
        _parameterMinimums[i] = minimums[i];
        _parameterMaximums[i] = maximums[i];
    }
}

void Live2DCoreRenderer::resetParameters()
{
    float* values = coreApi().getParameterValues(_model);
    if (!values)
    {
        return;
    }
    for (int i = 0; i < _parameterDefaults.size(); ++i)
    {
        values[i] = _parameterDefaults[i];
    }
}

void Live2DCoreRenderer::applyVisualState(const Live2DVisualState& state)
{
    const float gazeX = static_cast<float>((state.gazeX - 0.5) * 2.0);
    const float gazeY = static_cast<float>((0.5 - state.gazeY) * 2.0);
    const float idle = static_cast<float>(state.idlePhase);
    const float breathe = 0.5f + 0.5f * std::sin(idle * 1.7f);
    const float blink = std::sin(idle * 3.15f) > 0.982f ? 0.12f : 1.0f;
    const QString expression = state.expression.toLower();
    const QString emotion = state.emotion.toLower();
    const bool cheerful =
        expression.contains(QStringLiteral("smile")) || expression.contains(QStringLiteral("脸红")) ||
                                                                            emotion == QStringLiteral("cheerful");
    const bool angry =
        expression.contains(QStringLiteral("angry")) || expression.contains(QStringLiteral("生气")) ||
                                                                            emotion == QStringLiteral("angry");

    setParameter(QStringLiteral("ParamAngleX"), gazeX * 18.0f + std::sin(idle * 0.8f) * 3.0f);
    setParameter(QStringLiteral("ParamAngleY"), gazeY * 12.0f + std::sin(idle * 0.9f) * 2.0f);
    setParameter(QStringLiteral("ParamAngleZ"), -gazeX * 7.0f + std::sin(idle * 0.55f) * 2.0f);
    setParameter(QStringLiteral("ParamBodyAngleX"), gazeX * 8.0f);
    setParameter(QStringLiteral("ParamBodyAngleY"), gazeY * 4.0f);
    setParameter(QStringLiteral("ParamBodyAngleZ"), -gazeX * 4.0f + std::sin(idle * 0.7f) * 1.4f);
    setParameter(QStringLiteral("ParamBreath"), breathe);
    setParameter(QStringLiteral("ParamEyeBallX"), gazeX);
    setParameter(QStringLiteral("ParamEyeBallY"), gazeY);
    setParameter(QStringLiteral("ParamEyeLOpen"), blink);
    setParameter(QStringLiteral("ParamEyeROpen"), blink);
    setParameter(QStringLiteral("ParamMouthForm"), cheerful ? 0.8f : (angry ? -0.55f : 0.18f));

    const QString motion = state.motion.toLower();
    const float speakingPulse = (motion == QStringLiteral("talk") || motion == QStringLiteral("speak"))
                                 ? (0.5f + 0.5f * std::sin(idle * 16.0f)) * 0.85f
                                 : 0.0f;
    setParameter(QStringLiteral("ParamMouthOpenY"),
                                clamped(static_cast<float>(state.lipSyncValue) + speakingPulse, 0.0f, 1.0f));
    setParameter(QStringLiteral("ParamCheek"), cheerful ? 1.0f : 0.0f);
    setParameter(QStringLiteral("lianhong"), cheerful ? 1.0f : 0.0f);
    setParameter(QStringLiteral("shrngqi"), angry ? 1.0f : 0.0f);
}

void Live2DCoreRenderer::setParameter(const QString& id, float value, float weight)
{
    const auto it = _parameterIndexById.constFind(id);
    if (it == _parameterIndexById.constEnd())
    {
        return;
    }

    const int index = it.value();
    float* values = coreApi().getParameterValues(_model);
    if (!values || index < 0 || index >= _parameterDefaults.size())
    {
        return;
    }

    values[index] =
        clamped(blended(values[index], value, weight), _parameterMinimums[index], _parameterMaximums[index]);
}

QRectF Live2DCoreRenderer::currentModelBounds(bool visibleOnly) const
{
    CoreApi& api = coreApi();
    const int drawableCount = api.getDrawableCount(_model);
    const unsigned char* dynamicFlags = api.getDrawableDynamicFlags(_model);
    const int* vertexCounts = api.getDrawableVertexCounts(_model);
    const CsmVector2** positions = api.getDrawableVertexPositions(_model);

    bool hasPoint = false;
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    for (int drawable = 0; drawable < drawableCount; ++drawable)
    {
        if (visibleOnly && dynamicFlags && !(dynamicFlags[drawable] & DrawableFlagIsVisible))
        {
            continue;
        }
        const int vertexCount = vertexCounts[drawable];
        const CsmVector2* vertices = positions[drawable];
        for (int i = 0; i < vertexCount; ++i)
        {
            if (!hasPoint)
            {
                minX = maxX = vertices[i].X;
                minY = maxY = vertices[i].Y;
                hasPoint = true;
            }
            else
            {
                minX = std::min(minX, vertices[i].X);
                minY = std::min(minY, vertices[i].Y);
                maxX = std::max(maxX, vertices[i].X);
                maxY = std::max(maxY, vertices[i].Y);
            }
        }
    }
    if (!hasPoint)
    {
        return {};
    }
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).normalized();
}

void Live2DCoreRenderer::paintModel(QPainter* painter, const QRectF& stage)
{
    if (stage.width() <= 0 || stage.height() <= 0)
    {
        return;
    }

    CoreApi& api = coreApi();
    QRectF modelBounds = currentModelBounds(true);
    if (modelBounds.isEmpty())
    {
        modelBounds = currentModelBounds(false);
    }
    if (modelBounds.isEmpty())
    {
        return;
    }

    const QRect renderRect = stage.toAlignedRect();
    if (renderRect.isEmpty())
    {
        return;
    }
    QImage layer(renderRect.size(), QImage::Format_ARGB32);
    layer.fill(Qt::transparent);

    const qreal scale = qMin(stage.width() / modelBounds.width(), stage.height() / modelBounds.height()) * 0.96;
    const qreal tx = stage.center().x() - modelBounds.center().x() * scale;
    const qreal ty = stage.center().y() + modelBounds.center().y() * scale;
    const QPointF renderOrigin = renderRect.topLeft();

    const auto mapPoint = [scale, tx, ty, renderOrigin](const CsmVector2& point)
    {
        return QPointF(tx + point.X * scale, ty - point.Y * scale) - renderOrigin;
    };

    const int drawableCount = api.getDrawableCount(_model);
    const int* renderOrders = api.getRenderOrders(_model);
    const unsigned char* constantFlags = api.getDrawableConstantFlags(_model);
    const unsigned char* dynamicFlags = api.getDrawableDynamicFlags(_model);
    const int* blendModes = api.getDrawableBlendModes(_model);
    const int* maskCounts = api.getDrawableMaskCounts(_model);
    const int** masks = api.getDrawableMasks(_model);
    const int* textureIndices = api.getDrawableTextureIndices(_model);
    const int* vertexCounts = api.getDrawableVertexCounts(_model);
    const CsmVector2** positions = api.getDrawableVertexPositions(_model);
    const CsmVector2** uvs = api.getDrawableVertexUvs(_model);
    const int* indexCounts = api.getDrawableIndexCounts(_model);
    const unsigned short** indices = api.getDrawableIndices(_model);
    const float* opacities = api.getDrawableOpacities(_model);
    const CsmVector4* multiplyColors = api.getDrawableMultiplyColors ? api.getDrawableMultiplyColors(_model) : nullptr;
    const CsmVector4* screenColors = api.getDrawableScreenColors ? api.getDrawableScreenColors(_model) : nullptr;

    QVector<int> drawables(drawableCount);
    for (int i = 0; i < drawableCount; ++i)
    {
        drawables[i] = i;
    }
    std::stable_sort(drawables.begin(),
                     drawables.end(),
                     [renderOrders](int lhs, int rhs)
                     {
                         return renderOrders[lhs] < renderOrders[rhs];
                     });

    for (int drawable : drawables)
    {
        if (dynamicFlags && !(dynamicFlags[drawable] & DrawableFlagIsVisible))
        {
            continue;
        }
        const int textureIndex = textureIndices[drawable];
        if (textureIndex < 0 || textureIndex >= _textures.size())
        {
            continue;
        }
        const QImage& texture = _textures[textureIndex];
        if (texture.isNull())
        {
            continue;
        }

        const qreal opacity = opacities ? qBound<qreal>(0.0, opacities[drawable], 1.0) : 1.0;
        if (opacity <= 0.001)
        {
            continue;
        }
        const int blendMode = blendModes ? blendModes[drawable] : CubismBlendModeNormal;
        const bool invertedMask = constantFlags && (constantFlags[drawable] & DrawableFlagIsInvertedMask);
        ColorTransform transform;
        if (multiplyColors)
        {
            transform.multiplyRed = qBound<qreal>(0.0, multiplyColors[drawable].X, 1.0);
            transform.multiplyGreen = qBound<qreal>(0.0, multiplyColors[drawable].Y, 1.0);
            transform.multiplyBlue = qBound<qreal>(0.0, multiplyColors[drawable].Z, 1.0);
        }
        if (screenColors)
        {
            transform.screenRed = qBound<qreal>(0.0, screenColors[drawable].X, 1.0);
            transform.screenGreen = qBound<qreal>(0.0, screenColors[drawable].Y, 1.0);
            transform.screenBlue = qBound<qreal>(0.0, screenColors[drawable].Z, 1.0);
        }

        const int vertexCount = vertexCounts[drawable];
        const int indexCount = indexCounts[drawable];
        const CsmVector2* vertexPositions = positions[drawable];
        const CsmVector2* vertexUvs = uvs[drawable];
        const unsigned short* drawableIndices = indices[drawable];
        QImage clipMask;
        const int maskCount = maskCounts ? maskCounts[drawable] : 0;
        if (maskCount > 0 && masks && masks[drawable])
        {
            clipMask = QImage(renderRect.size(), QImage::Format_Alpha8);
            clipMask.fill(0);

            for (int maskSlot = 0; maskSlot < maskCount; ++maskSlot)
            {
                const int maskDrawable = masks[drawable][maskSlot];
                if (maskDrawable < 0 || maskDrawable >= drawableCount)
                {
                    continue;
                }
                if (dynamicFlags && !(dynamicFlags[maskDrawable] & DrawableFlagIsVisible))
                {
                    continue;
                }
                const int maskTextureIndex = textureIndices[maskDrawable];
                if (maskTextureIndex < 0 || maskTextureIndex >= _textures.size())
                {
                    continue;
                }
                const QImage& maskTexture = _textures[maskTextureIndex];
                if (maskTexture.isNull())
                {
                    continue;
                }

                const int maskVertexCount = vertexCounts[maskDrawable];
                const int maskIndexCount = indexCounts[maskDrawable];
                const CsmVector2* maskVertexPositions = positions[maskDrawable];
                const CsmVector2* maskVertexUvs = uvs[maskDrawable];
                const unsigned short* maskDrawableIndices = indices[maskDrawable];
                const qreal maskOpacity = opacities ? qBound<qreal>(0.0, opacities[maskDrawable], 1.0) : 1.0;
                if (maskOpacity <= 0.001)
                {
                    continue;
                }

                for (int i = 0; i + 2 < maskIndexCount; i += 3)
                {
                    const int i0 = maskDrawableIndices[i];
                    const int i1 = maskDrawableIndices[i + 1];
                    const int i2 = maskDrawableIndices[i + 2];
                    if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= maskVertexCount || i1 >= maskVertexCount ||
                        i2 >= maskVertexCount)
                    {
                        continue;
                    }

                    const QPointF p0 = mapPoint(maskVertexPositions[i0]);
                    const QPointF p1 = mapPoint(maskVertexPositions[i1]);
                    const QPointF p2 = mapPoint(maskVertexPositions[i2]);
                    const QPointF t0(maskVertexUvs[i0].X * maskTexture.width(),
                                     (1.0f - maskVertexUvs[i0].Y) * maskTexture.height());
                    const QPointF t1(maskVertexUvs[i1].X * maskTexture.width(),
                                     (1.0f - maskVertexUvs[i1].Y) * maskTexture.height());
                    const QPointF t2(maskVertexUvs[i2].X * maskTexture.width(),
                                     (1.0f - maskVertexUvs[i2].Y) * maskTexture.height());
                    paintMaskTriangle(clipMask, maskTexture, p0, p1, p2, t0, t1, t2, maskOpacity);
                }
            }
        }
        const QImage* clipMaskPtr = clipMask.isNull() ? nullptr : &clipMask;

        for (int i = 0; i + 2 < indexCount; i += 3)
        {
            const int i0 = drawableIndices[i];
            const int i1 = drawableIndices[i + 1];
            const int i2 = drawableIndices[i + 2];
            if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount)
            {
                continue;
            }

            const QPointF p0 = mapPoint(vertexPositions[i0]);
            const QPointF p1 = mapPoint(vertexPositions[i1]);
            const QPointF p2 = mapPoint(vertexPositions[i2]);
            const QPointF t0(vertexUvs[i0].X * texture.width(), (1.0f - vertexUvs[i0].Y) * texture.height());
            const QPointF t1(vertexUvs[i1].X * texture.width(), (1.0f - vertexUvs[i1].Y) * texture.height());
            const QPointF t2(vertexUvs[i2].X * texture.width(), (1.0f - vertexUvs[i2].Y) * texture.height());
            QRectF sourceBounds(t0, QSizeF(0, 0));
            sourceBounds |= QRectF(t1, QSizeF(0, 0));
            sourceBounds |= QRectF(t2, QSizeF(0, 0));
            sourceBounds = sourceBounds.normalized()
                               .adjusted(-1.0, -1.0, 1.0, 1.0)
                               .intersected(QRectF(QPointF(0, 0), texture.size()));
            if (sourceBounds.isEmpty())
            {
                continue;
            }

            paintTexturedTriangle(layer,
                                  texture,
                                  p0,
                                  p1,
                                  p2,
                                  t0,
                                  t1,
                                  t2,
                                  opacity,
                                  blendMode,
                                  transform,
                                  clipMaskPtr,
                                  invertedMask);
        }
    }

    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter->drawImage(renderRect.topLeft(), layer.convertToFormat(QImage::Format_ARGB32_Premultiplied));
    painter->restore();
}
