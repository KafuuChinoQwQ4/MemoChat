#ifndef LIVE2DCORERENDERER_H
#define LIVE2DCORERENDERER_H

#include "Live2DRenderer.h"

#include <QHash>
#include <QImage>
#include <QSize>
#include <QString>
#include <QVector>
#include <memory>

struct csmMoc;
struct csmModel;

class Live2DCoreRenderer final : public Live2DRenderer
{
public:
    explicit Live2DCoreRenderer(const QString &modelPath = QString());
    ~Live2DCoreRenderer() override;

    bool isReady() const;
    QString errorString() const;

    QString rendererName() const override;
    bool isNative() const override;
    void paint(QPainter *painter, const QRectF &bounds, const Live2DVisualState &state) override;
    QImage renderToImage(const QSize &size, const Live2DVisualState &state);

private:
    struct AlignedBuffer;
    struct CoreApi;

    bool loadDefaultModel();
    bool loadModelFile(const QString &modelPath);
    bool loadMoc(const QString &mocPath);
    bool loadTextures(const QString &modelDirectory, const QVector<QString> &texturePaths);
    void initializeParameters();
    void resetParameters();
    void applyVisualState(const Live2DVisualState &state);
    void setParameter(const QString &id, float value, float weight = 1.0f);
    QRectF currentModelBounds(bool visibleOnly) const;
    void paintModel(QPainter *painter, const QRectF &stage);

    static CoreApi &coreApi();

    bool _ready = false;
    QString _error;
    std::unique_ptr<AlignedBuffer> _mocBuffer;
    std::unique_ptr<AlignedBuffer> _modelBuffer;
    csmMoc *_moc = nullptr;
    csmModel *_model = nullptr;
    QVector<QImage> _textures;
    QHash<QString, int> _parameterIndexById;
    QVector<float> _parameterDefaults;
    QVector<float> _parameterMinimums;
    QVector<float> _parameterMaximums;
};

#endif // LIVE2DCORERENDERER_H
