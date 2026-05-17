#ifndef LIVE2DOFFICIALOPENGLRENDERER_H
#define LIVE2DOFFICIALOPENGLRENDERER_H

#include "Live2DRenderer.h"

#include <QSize>
#include <QString>
#include <memory>

class Live2DOfficialOpenGLRenderer final
{
public:
    explicit Live2DOfficialOpenGLRenderer(const QString &modelPath = QString(),
                                          const QString &motionDirectory = QString(),
                                          const QString &expressionDirectory = QString());
    ~Live2DOfficialOpenGLRenderer();

    bool isReady() const;
    QString errorString() const;
    QString rendererName() const;
    bool isNative() const;

    bool render(const QSize &viewportSize, const Live2DVisualState &state);

    static QString defaultModelPath();

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

#endif // LIVE2DOFFICIALOPENGLRENDERER_H
