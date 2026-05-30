#ifndef LIVE2DAVATAROPENGLRENDERER_H
#define LIVE2DAVATAROPENGLRENDERER_H

#include <QImage>
#include <QSize>
#include <QString>

class Live2DAvatarOpenGLRenderer final
{
public:
    Live2DAvatarOpenGLRenderer() = default;

    QImage renderAvatar(const QString& modelJsonPath, QString* error = nullptr) const;

    static QString rendererName();

private:
    static QImage cropAvatarFrame(const QImage& source, const QSize& outputSize);
};

#endif // LIVE2DAVATAROPENGLRENDERER_H
