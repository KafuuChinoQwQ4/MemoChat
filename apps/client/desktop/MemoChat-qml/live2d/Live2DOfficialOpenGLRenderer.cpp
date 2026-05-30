#include "Live2DOfficialOpenGLRenderer.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QRectF>
#include <QSet>
#include <QUrl>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

#if MEMOCHAT_ENABLE_LIVE2D_NATIVE && MEMOCHAT_LIVE2D_CUBISM_FOUND
#include <GL/glew.h>

#include <CubismFramework.hpp>
#include <Id/CubismIdManager.hpp>
#include <Math/CubismMatrix44.hpp>
#include <Model/CubismMoc.hpp>
#include <Model/CubismModel.hpp>
#include <Motion/ACubismMotion.hpp>
#include <Motion/CubismExpressionMotion.hpp>
#include <Motion/CubismExpressionMotionManager.hpp>
#include <Motion/CubismMotion.hpp>
#include <Motion/CubismMotionManager.hpp>
#include <Physics/CubismPhysics.hpp>
#include <Rendering/CubismRenderer.hpp>
#include <Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp>

#if defined(__linux__)
#include <GL/glx.h>
#endif

#if defined(_MSC_VER)
#include <malloc.h>
#endif
#endif

namespace
{

QString clientSourcePath(const QString& relativePath)
{
#ifdef MEMOCHAT_QML_SOURCE_DIR
    const QString root = QString::fromUtf8(MEMOCHAT_QML_SOURCE_DIR);
#else
    const QString root = QCoreApplication::applicationDirPath();
#endif
    return QDir(root).absoluteFilePath(relativePath);
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

QString resolveModelPath(const QString& modelPath)
{
    const QString cleaned = modelPath.trimmed();
    if (cleaned.isEmpty())
    {
        return Live2DOfficialOpenGLRenderer::defaultModelPath();
    }
    if (cleaned.startsWith(QStringLiteral("qrc:/")))
    {
        return QStringLiteral(":") + QUrl(cleaned).path();
    }
    if (cleaned.startsWith(QStringLiteral(":/")))
    {
        return QDir::cleanPath(cleaned);
    }

    const QUrl url(cleaned);
    if (url.isLocalFile())
    {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (QDir::isAbsolutePath(cleaned))
    {
        return QDir::cleanPath(cleaned);
    }
    return QDir::cleanPath(QDir::current().absoluteFilePath(cleaned));
}

float clamped(float value, float minimum, float maximum)
{
    return std::max(minimum, std::min(maximum, value));
}

QString normalizedKey(QString value)
{
    return value.trimmed().toLower();
}

#if MEMOCHAT_ENABLE_LIVE2D_NATIVE && MEMOCHAT_LIVE2D_CUBISM_FOUND
namespace Csm = Live2D::Cubism::Framework;
namespace CsmRendering = Live2D::Cubism::Framework::Rendering;

QMutex& cubismRenderMutex()
{
    static QMutex mutex;
    return mutex;
}

bool hasCurrentOpenGLContext()
{
#if defined(__linux__)
    return glXGetCurrentContext() != nullptr;
#else
    return true;
#endif
}

QString resolveAssetDirectory(const QString& inputDirectory, const QDir& modelDir)
{
    const QString cleaned = inputDirectory.trimmed();
    if (cleaned.isEmpty())
    {
        return modelDir.absolutePath();
    }
    const QUrl url(cleaned);
    if (url.isLocalFile())
    {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (QDir::isAbsolutePath(cleaned) || cleaned.startsWith(QStringLiteral(":/")))
    {
        return QDir::cleanPath(cleaned);
    }
    const QString modelRelative = modelDir.absoluteFilePath(cleaned);
    if (QFileInfo::exists(modelRelative))
    {
        return QDir::cleanPath(modelRelative);
    }
    return QDir::cleanPath(QDir::current().absoluteFilePath(cleaned));
}

void collectFilesRecursive(const QDir& dir, const QStringList& nameFilters, QStringList* files)
{
    if (!files || !dir.exists())
    {
        return;
    }

    const QFileInfoList entries =
        dir.entryInfoList(nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& entry : entries)
    {
        files->append(entry.absoluteFilePath());
    }

    const QFileInfoList childDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& childDir : childDirs)
    {
        collectFilesRecursive(QDir(childDir.absoluteFilePath()), nameFilters, files);
    }
}

QString live2dFileStem(const QString& path)
{
    const QString fileName = QFileInfo(path).fileName();
    const QString lowerName = fileName.toLower();
    if (lowerName.endsWith(QStringLiteral(".motion3.json")))
    {
        return fileName.left(fileName.size() - QStringLiteral(".motion3.json").size());
    }
    if (lowerName.endsWith(QStringLiteral(".exp3.json")))
    {
        return fileName.left(fileName.size() - QStringLiteral(".exp3.json").size());
    }
    return QFileInfo(path).completeBaseName();
}

QString resolvedAssetPath(const QString& fileName, const QDir& modelDir)
{
    const QString trimmed = fileName.trimmed();
    if (trimmed.isEmpty())
    {
        return {};
    }
    if (QFileInfo(trimmed).isAbsolute() || trimmed.startsWith(QStringLiteral(":/")) ||
                                                              trimmed.startsWith(QStringLiteral("qrc:/")))
    {
        return QDir::cleanPath(trimmed.startsWith(QStringLiteral("qrc:/")) ? QStringLiteral(":") + QUrl(trimmed).path()
                                                                           : trimmed);
    }
    return QDir::cleanPath(modelDir.absoluteFilePath(trimmed));
}

class QtCubismAllocator final : public Csm::ICubismAllocator
{
public:
    void* Allocate(const Csm::csmSizeType size) override
    {
        return std::malloc(size);
    }

    void Deallocate(void* memory) override
    {
        std::free(memory);
    }

    void* AllocateAligned(const Csm::csmSizeType size, const Csm::csmUint32 alignment) override
    {
        void* memory = nullptr;
#if defined(_MSC_VER)
        memory = _aligned_malloc(size, alignment);
#else
        if (posix_memalign(&memory, alignment, size) != 0)
        {
            memory = nullptr;
        }
#endif
        return memory;
    }

    void DeallocateAligned(void* alignedMemory) override
    {
#if defined(_MSC_VER)
        _aligned_free(alignedMemory);
#else
        std::free(alignedMemory);
#endif
    }
};

QString frameworkShaderPath(const std::string& filePath)
{
    const QString requested = QString::fromStdString(filePath);
    const QFileInfo requestedInfo(requested);
    if (requestedInfo.isAbsolute() || requested.startsWith(QStringLiteral(":/")))
    {
        return requested;
    }

#ifdef MEMOCHAT_LIVE2D_FRAMEWORK_INCLUDE_DIR
    const QString frameworkRoot = QString::fromUtf8(MEMOCHAT_LIVE2D_FRAMEWORK_INCLUDE_DIR);
#else
    const QString frameworkRoot;
#endif

    QString shaderName = requested;
    if (shaderName.startsWith(QStringLiteral("FrameworkShaders/")))
    {
        shaderName = shaderName.mid(QStringLiteral("FrameworkShaders/").size());
    }

    QStringList candidates;
    if (!frameworkRoot.isEmpty())
    {
        candidates << QDir(frameworkRoot)
                          .absoluteFilePath(QStringLiteral("Rendering/OpenGL/Shaders/Standard/%1").arg(shaderName));
        candidates << QDir(frameworkRoot).absoluteFilePath(requested);
    }
    candidates << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(requested);
    candidates << QDir::current().absoluteFilePath(requested);

    for (const QString& candidate : candidates)
    {
        if (QFileInfo::exists(candidate))
        {
            return candidate;
        }
    }
    return candidates.value(0, requested);
}

Csm::csmByte* loadCubismFile(const std::string filePath, Csm::csmSizeInt* outSize)
{
    if (outSize)
    {
        *outSize = 0;
    }

    QString error;
    const QByteArray bytes = readFileBytes(frameworkShaderPath(filePath), &error);
    if (bytes.isEmpty())
    {
        qWarning().noquote() << "Live2D Cubism file loader:" << error;
        return nullptr;
    }

    auto* buffer = static_cast<Csm::csmByte*>(std::malloc(static_cast<size_t>(bytes.size())));
    if (!buffer)
    {
        qWarning().noquote() << "Live2D Cubism file loader: allocation failed for" << QString::fromStdString(filePath);
        return nullptr;
    }
    std::memcpy(buffer, bytes.constData(), static_cast<size_t>(bytes.size()));
    if (outSize)
    {
        *outSize = static_cast<Csm::csmSizeInt>(bytes.size());
    }
    return buffer;
}

void releaseCubismBytes(Csm::csmByte* byteData)
{
    std::free(byteData);
}

void cubismLog(const char* message)
{
    if (!message)
    {
        return;
    }
    qInfo().noquote() << QString::fromUtf8(message).trimmed();
}

bool ensureCubismFramework(QString* error)
{
    static QtCubismAllocator allocator;
    static Csm::CubismFramework::Option option;

    if (!Csm::CubismFramework::IsStarted())
    {
        option.LogFunction = cubismLog;
        option.LoggingLevel = Csm::CubismFramework::Option::LogLevel_Warning;
        option.LoadFileFunction = loadCubismFile;
        option.ReleaseBytesFunction = releaseCubismBytes;
        if (!Csm::CubismFramework::StartUp(&allocator, &option))
        {
            if (error)
            {
                *error = QStringLiteral("failed to start Cubism Framework");
            }
            return false;
        }
    }

    if (!Csm::CubismFramework::IsInitialized())
    {
        Csm::CubismFramework::Initialize();
    }
    return Csm::CubismFramework::IsInitialized();
}

bool ensureGlew(QString* error)
{
    static bool attempted = false;
    static bool ready = false;
    if (attempted)
    {
        return ready;
    }
    attempted = true;

    glewExperimental = GL_TRUE;
    const GLenum result = glewInit();
    glGetError();
    ready = result == GLEW_OK;
    if (!ready && error)
    {
        *error = QStringLiteral("failed to initialize GLEW: %1") .arg(
            QString::fromUtf8(reinterpret_cast<const char*>(glewGetErrorString(result))));
    }
    return ready;
}

GLuint uploadTexture(const QString& path, QString* error)
{
    QImage image(path);
    if (image.isNull())
    {
        if (error)
        {
            *error = QStringLiteral("failed to load Live2D texture: %1").arg(path);
        }
        return 0;
    }

    image = image.convertToFormat(QImage::Format_RGBA8888);

    GLint previousActiveTexture = GL_TEXTURE0;
    GLint previousTextureBinding = 0;
    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTextureBinding);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 image.width(),
                 image.height(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 image.constBits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTextureBinding));
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    glActiveTexture(static_cast<GLenum>(previousActiveTexture));
    return textureId;
}
#endif
} // namespace

struct Live2DOfficialOpenGLRenderer::Impl
{
#if MEMOCHAT_ENABLE_LIVE2D_NATIVE && MEMOCHAT_LIVE2D_CUBISM_FOUND
    ~Impl()
    {
        release();
    }

    bool
    load(const QString& inputModelPath, const QString& inputMotionDirectory, const QString& inputExpressionDirectory)
    {
        QMutexLocker locker(&cubismRenderMutex());

        QString frameworkError;
        if (!ensureCubismFramework(&frameworkError))
        {
            error = frameworkError;
            return false;
        }
        if (!ensureGlew(&frameworkError))
        {
            error = frameworkError;
            return false;
        }
        motionManager = CSM_NEW Csm::CubismMotionManager();
        expressionManager = CSM_NEW Csm::CubismExpressionMotionManager();

        modelPath = resolveModelPath(inputModelPath);
        QString fileError;
        const QByteArray modelBytes = readFileBytes(modelPath, &fileError);
        if (modelBytes.isEmpty())
        {
            error = fileError;
            return false;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(modelBytes);
        const QJsonObject root = doc.object();
        const QJsonObject fileReferences = root.value(QStringLiteral("FileReferences")).toObject();
        const QString mocName = fileReferences.value(QStringLiteral("Moc")).toString();
        const QJsonArray textures = fileReferences.value(QStringLiteral("Textures")).toArray();
        if (mocName.isEmpty() || textures.isEmpty())
        {
            error = QStringLiteral("model3.json is missing Moc or Textures: %1").arg(modelPath);
            return false;
        }

        const QDir modelDir(QFileInfo(modelPath).absolutePath());
        motionDirectory = resolveAssetDirectory(inputMotionDirectory, modelDir);
        expressionDirectory = resolveAssetDirectory(inputExpressionDirectory, modelDir);
        const QString mocPath = modelDir.absoluteFilePath(mocName);
        const QByteArray mocBytes = readFileBytes(mocPath, &fileError);
        if (mocBytes.isEmpty())
        {
            error = fileError;
            return false;
        }

        moc = Csm::CubismMoc::Create(reinterpret_cast<const Csm::csmByte*>(mocBytes.constData()),
                                     static_cast<Csm::csmSizeInt>(mocBytes.size()));
        if (!moc)
        {
            error = QStringLiteral("failed to create Cubism moc: %1").arg(mocPath);
            return false;
        }

        model = moc->CreateModel();
        if (!model)
        {
            error = QStringLiteral("failed to create Cubism model: %1").arg(mocPath);
            return false;
        }

        resetSpecialExpressionParameters();
        loadPhysics(fileReferences, modelDir);
        loadMotions(fileReferences, modelDir, QDir(motionDirectory));
        loadExpressions(fileReferences, modelDir, QDir(expressionDirectory));
        model->SaveParameters();

        renderer = static_cast<CsmRendering::CubismRenderer_OpenGLES2*>(CsmRendering::CubismRenderer::Create(1, 1));
        if (!renderer)
        {
            error = QStringLiteral("failed to create official Cubism OpenGL renderer");
            return false;
        }
        renderer->Initialize(model);
        renderer->IsPremultipliedAlpha(false);
        renderer->IsCulling(false);

        for (int i = 0; i < textures.size(); ++i)
        {
            const QString textureName = textures.at(i).toString();
            if (textureName.isEmpty())
            {
                continue;
            }
            const GLuint textureId = uploadTexture(modelDir.absoluteFilePath(textureName), &fileError);
            if (textureId == 0)
            {
                error = fileError;
                return false;
            }
            textureIds.push_back(textureId);
            renderer->BindTexture(static_cast<Csm::csmUint32>(i), textureId);
        }

        ready = true;
        return true;
    }

    void release()
    {
        QMutexLocker locker(&cubismRenderMutex());

        if (!textureIds.isEmpty())
        {
            if (hasCurrentOpenGLContext())
            {
                glDeleteTextures(textureIds.size(), textureIds.constData());
            }
            textureIds.clear();
        }
        if (renderer)
        {
            CsmRendering::CubismRenderer::Delete(renderer);
            renderer = nullptr;
        }
        if (expressionManager)
        {
            expressionManager->StopAllMotions();
            if (currentExpression)
            {
                currentExpression->SetLoop(expressionLoopDefaults.value(currentExpression, false));
            }
            CSM_DELETE(expressionManager);
            expressionManager = nullptr;
        }
        if (motionManager)
        {
            motionManager->StopAllMotions();
            if (currentMotion)
            {
                currentMotion->SetLoop(motionLoopDefaults.value(currentMotion, false));
            }
            CSM_DELETE(motionManager);
            motionManager = nullptr;
        }
        if (moc && model)
        {
            moc->DeleteModel(model);
            model = nullptr;
        }
        if (physics)
        {
            Csm::CubismPhysics::Delete(physics);
            physics = nullptr;
        }
        for (auto* motion : expressions)
        {
            Csm::ACubismMotion::Delete(motion);
        }
        expressions.clear();
        expressionAliases.clear();
        expressionLoopDefaults.clear();
        for (auto* motion : motions)
        {
            Csm::ACubismMotion::Delete(motion);
        }
        motions.clear();
        motionAliases.clear();
        motionLoopDefaults.clear();
        currentExpression = nullptr;
        lastExpressionSerial = -1;
        currentMotion = nullptr;
        currentMotionKey.clear();
        currentMotionPersistent = false;
        lastActionSerial = -1;
        currentExpressionPersistent = false;
        hasLastIdlePhase = false;
        if (moc)
        {
            Csm::CubismMoc::Delete(moc);
            moc = nullptr;
        }
        ready = false;
    }

    bool render(const QSize& viewportSize, const Live2DVisualState& state)
    {
        QMutexLocker locker(&cubismRenderMutex());

        if (!ready || !model || !renderer || viewportSize.width() <= 0 || viewportSize.height() <= 0)
        {
            return false;
        }

        glViewport(0, 0, viewportSize.width(), viewportSize.height());
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const float deltaSeconds = frameDeltaSeconds(state.idlePhase);
        model->LoadParameters();
        applyVisualState(state);
        applyMotion(state, deltaSeconds);
        applyExpression(state);
        if (physics)
        {
            physics->Evaluate(model, deltaSeconds);
        }
        model->Update();

        renderer->SetRenderTargetSize(static_cast<Csm::csmUint32>(viewportSize.width()),
                                      static_cast<Csm::csmUint32>(viewportSize.height()));
        Csm::CubismMatrix44 mvp = fittedMvp(viewportSize);
        renderer->SetMvpMatrix(&mvp);
        renderer->DrawModel();
        return true;
    }

    float frameDeltaSeconds(qreal idlePhase)
    {
        const float current = static_cast<float>(idlePhase);
        float delta = 1.0f / 60.0f;
        if (hasLastIdlePhase)
        {
            delta = clamped(current - lastIdlePhase, 1.0f / 240.0f, 1.0f / 15.0f);
        }
        lastIdlePhase = current;
        hasLastIdlePhase = true;
        return delta;
    }

    void loadPhysics(const QJsonObject& fileReferences, const QDir& modelDir)
    {
        const QString physicsName = fileReferences.value(QStringLiteral("Physics")).toString();
        if (physicsName.isEmpty())
        {
            return;
        }

        QString fileError;
        const QString physicsPath = modelDir.absoluteFilePath(physicsName);
        const QByteArray physicsBytes = readFileBytes(physicsPath, &fileError);
        if (physicsBytes.isEmpty())
        {
            qWarning().noquote() << "Live2D physics unavailable:" << fileError;
            return;
        }
        physics = Csm::CubismPhysics::Create(reinterpret_cast<const Csm::csmByte*>(physicsBytes.constData()),
                                             static_cast<Csm::csmSizeInt>(physicsBytes.size()));
        if (!physics)
        {
            qWarning().noquote() << "Live2D physics unavailable: failed to parse" << physicsPath;
        }
    }

    void loadMotions(const QJsonObject& fileReferences, const QDir& modelDir, const QDir& motionDir)
    {
        QSet<QString> declaredFiles;
        const QJsonObject motionGroups = fileReferences.value(QStringLiteral("Motions")).toObject();
        for (auto group = motionGroups.constBegin(); group != motionGroups.constEnd(); ++group)
        {
            const QJsonArray items = group.value().toArray();
            for (int i = 0; i < items.size(); ++i)
            {
                const QJsonObject item = items.at(i).toObject();
                declaredFiles.insert(resolvedAssetPath(item.value(QStringLiteral("File")).toString(), modelDir));
                addMotion(group.key(),
                          i,
                          item.value(QStringLiteral("Name")).toString(),
                                     item.value(QStringLiteral("File")).toString(), modelDir);
            }
        }

        QStringList files;
        collectFilesRecursive(motionDir, {QStringLiteral("*.motion3.json")}, &files);
        int directoryIndex = 0;
        for (const QString& file : files)
        {
            if (declaredFiles.contains(QDir::cleanPath(file)))
            {
                continue;
            }
            addMotion(QStringLiteral("Directory"), directoryIndex, live2dFileStem(file), file, modelDir);
            ++directoryIndex;
        }
    }

    void addMotion(const QString& group, int index, const QString& name, const QString& fileName, const QDir& modelDir)
    {
        const QString trimmedGroup = group.trimmed();
        const QString trimmedFile = fileName.trimmed();
        if (trimmedFile.isEmpty())
        {
            return;
        }

        const QString key = motionKey(trimmedGroup, index);
        if (key.isEmpty() || motions.contains(key))
        {
            return;
        }

        QString fileError;
        const QString motionPath =
            QFileInfo(trimmedFile).isAbsolute() ? trimmedFile : modelDir.absoluteFilePath(trimmedFile);
        const QByteArray motionBytes = readFileBytes(motionPath, &fileError);
        if (motionBytes.isEmpty())
        {
            return;
        }

        auto* motion = Csm::CubismMotion::Create(reinterpret_cast<const Csm::csmByte*>(motionBytes.constData()),
                                                 static_cast<Csm::csmSizeInt>(motionBytes.size()));
        if (!motion)
        {
            qWarning().noquote() << "Live2D motion unavailable: failed to parse" << motionPath;
            return;
        }
        motion->SetFadeInTime(0.16f);
        motion->SetFadeOutTime(0.22f);
        motions.insert(key, motion);
        motionLoopDefaults.insert(motion, motion->GetLoop());

        aliasMotion(trimmedGroup, key);
        aliasMotion(QStringLiteral("%1#%2").arg(trimmedGroup).arg(index), key);
        const QString trimmedName = name.trimmed();
        if (!trimmedName.isEmpty())
        {
            aliasMotion(trimmedName, key);
        }
        aliasMotion(live2dFileStem(trimmedFile), key);
    }

    void loadExpressions(const QJsonObject& fileReferences, const QDir& modelDir, const QDir& expressionDir)
    {
        QSet<QString> declaredFiles;
        const QJsonArray expressionItems = fileReferences.value(QStringLiteral("Expressions")).toArray();
        for (const QJsonValue& itemValue : expressionItems)
        {
            const QJsonObject item = itemValue.toObject();
            declaredFiles.insert(resolvedAssetPath(item.value(QStringLiteral("File")).toString(), modelDir));
            addExpression(item.value(QStringLiteral("Name")).toString(),
                                     item.value(QStringLiteral("File")).toString(), modelDir);
        }

        addExpression(QStringLiteral("脸红"), QStringLiteral("脸红.exp3.json"), modelDir);
        addExpression(QStringLiteral("脸黑"), QStringLiteral("脸黑.exp3.json"), modelDir);
        addExpression(QStringLiteral("提比"), QStringLiteral("提比.exp3.json"), modelDir);

        QStringList files;
        collectFilesRecursive(expressionDir, {QStringLiteral("*.exp3.json")}, &files);
        for (const QString& file : files)
        {
            if (declaredFiles.contains(QDir::cleanPath(file)))
            {
                continue;
            }
            addExpression(live2dFileStem(file), file, modelDir);
        }

        aliasExpression(QStringLiteral("smile"), QStringLiteral("脸红"));
        aliasExpression(QStringLiteral("smile_soft"), QStringLiteral("脸红"));
        aliasExpression(QStringLiteral("happy"), QStringLiteral("脸红"));
        aliasExpression(QStringLiteral("cheerful"), QStringLiteral("脸红"));
        aliasExpression(QStringLiteral("blush"), QStringLiteral("脸红"));
        aliasExpression(QStringLiteral("angry"), QStringLiteral("脸黑"));
        aliasExpression(QStringLiteral("脸黑"), QStringLiteral("脸黑"));
        aliasExpression(QStringLiteral("生气"), QStringLiteral("脸黑"));
        aliasExpression(QStringLiteral("tibi"), QStringLiteral("提比"));
    }

    void addExpression(const QString& name, const QString& fileName, const QDir& modelDir)
    {
        const QString trimmedName = name.trimmed();
        const QString trimmedFile = fileName.trimmed();
        if (trimmedFile.isEmpty())
        {
            return;
        }

        const QString key = normalizedKey(trimmedName.isEmpty() ? live2dFileStem(trimmedFile) : trimmedName);
        if (key.isEmpty() || expressions.contains(key))
        {
            return;
        }

        QString fileError;
        const QString expressionPath =
            QFileInfo(trimmedFile).isAbsolute() ? trimmedFile : modelDir.absoluteFilePath(trimmedFile);
        const QByteArray expressionBytes = readFileBytes(expressionPath, &fileError);
        if (expressionBytes.isEmpty())
        {
            return;
        }

        auto* motion =
            Csm::CubismExpressionMotion::Create(reinterpret_cast<const Csm::csmByte*>(expressionBytes.constData()),
                                                static_cast<Csm::csmSizeInt>(expressionBytes.size()));
        if (!motion)
        {
            qWarning().noquote() << "Live2D expression unavailable: failed to parse" << expressionPath;
            return;
        }
        motion->SetFadeInTime(0.12f);
        motion->SetFadeOutTime(0.16f);
        expressions.insert(key, motion);
        expressionLoopDefaults.insert(motion, motion->GetLoop());
    }

    void aliasExpression(const QString& alias, const QString& target)
    {
        const QString targetKey = normalizedKey(target);
        if (!expressions.contains(targetKey))
        {
            return;
        }
        expressionAliases.insert(normalizedKey(alias), targetKey);
    }

    QString motionKey(const QString& group, int index) const
    {
        const QString normalizedGroup = normalizedKey(group);
        if (normalizedGroup.isEmpty() || index < 0)
        {
            return {};
        }
        return QStringLiteral("%1#%2").arg(normalizedGroup).arg(index);
    }

    void aliasMotion(const QString& alias, const QString& targetKey)
    {
        const QString aliasKey = normalizedKey(alias);
        if (aliasKey.isEmpty() || targetKey.isEmpty() || !motions.contains(targetKey))
        {
            return;
        }
        motionAliases.insert(aliasKey, targetKey);
    }

    Csm::ACubismMotion* motionForState(const Live2DVisualState& state, QString* resolvedKey) const
    {
        QString key = normalizedKey(state.motion);
        if (key.isEmpty())
        {
            if (resolvedKey)
            {
                resolvedKey->clear();
            }
            return nullptr;
        }
        key = motionAliases.value(key, key);
        if (resolvedKey)
        {
            *resolvedKey = key;
        }
        return motions.value(key, nullptr);
    }

    void applyMotion(const Live2DVisualState& state, float deltaSeconds)
    {
        if (!motionManager)
        {
            return;
        }

        QString nextMotionKey;
        auto* nextMotion = motionForState(state, &nextMotionKey);
        const bool triggerChanged = state.actionSerial != lastActionSerial;
        const bool persistenceChanged = state.persistentMotion != currentMotionPersistent;
        if (triggerChanged || persistenceChanged || nextMotion != currentMotion || nextMotionKey != currentMotionKey)
        {
            motionManager->StopAllMotions();
            if (currentMotion)
            {
                currentMotion->SetLoop(motionLoopDefaults.value(currentMotion, false));
            }
            currentMotion = nextMotion;
            currentMotionKey = nextMotionKey;
            currentMotionPersistent = state.persistentMotion;
            lastActionSerial = state.actionSerial;
            if (currentMotion)
            {
                currentMotion->SetLoop(currentMotionPersistent || motionLoopDefaults.value(currentMotion, false));
                motionManager->StartMotionPriority(currentMotion, false, 3);
            }
        }
        motionManager->UpdateMotion(model, deltaSeconds);
    }

    Csm::ACubismMotion* expressionForState(const Live2DVisualState& state) const
    {
        QString key = normalizedKey(state.expression);
        const QString emotion = normalizedKey(state.emotion);
        if (key == QStringLiteral("neutral") || key == QStringLiteral("focus"))
        {
            key.clear();
        }
        if (key.isEmpty() && emotion != QStringLiteral("neutral") && emotion != QStringLiteral("speaking"))
        {
            key = emotion;
        }
        if (key.isEmpty())
        {
            return nullptr;
        }
        key = expressionAliases.value(key, key);
        return expressions.value(key, nullptr);
    }

    void applyExpression(const Live2DVisualState& state)
    {
        if (!expressionManager)
        {
            return;
        }
        auto* nextExpression = expressionForState(state);
        const bool triggerChanged = state.actionSerial != lastExpressionSerial;
        const bool persistenceChanged = state.persistentMotion != currentExpressionPersistent;
        if (triggerChanged || persistenceChanged || nextExpression != currentExpression)
        {
            expressionManager->StopAllMotions();
            if (currentExpression)
            {
                currentExpression->SetLoop(expressionLoopDefaults.value(currentExpression, false));
            }
            currentExpression = nextExpression;
            currentExpressionPersistent = state.persistentMotion;
            lastExpressionSerial = state.actionSerial;
            if (currentExpression)
            {
                currentExpression->SetLoop(currentExpressionPersistent ||
                                           expressionLoopDefaults.value(currentExpression, false));
                expressionManager->StartMotion(currentExpression, false);
            }
        }
        expressionManager->UpdateMotion(model, 1.0f / 60.0f);
    }

    void applyVisualState(const Live2DVisualState& state)
    {
        resetSpecialExpressionParameters();

        const float gazeX = static_cast<float>((state.gazeX - 0.5) * 2.0);
        const float gazeY = static_cast<float>((0.5 - state.gazeY) * 2.0);
        const float idle = static_cast<float>(state.idlePhase);
        const float breathe = 0.5f + 0.5f * std::sin(idle * 1.7f);
        const float blink = std::sin(idle * 3.15f) > 0.982f ? 0.12f : 1.0f;
        const QString expression = state.expression.toLower();
        const QString emotion = state.emotion.toLower();
        const bool cheerful = expression.contains(
            QStringLiteral("smile")) ||
            expression.contains(QStringLiteral("脸红")) ||
                                emotion == QStringLiteral("happy") || emotion == QStringLiteral("cheerful");
        const bool angry = expression.contains(
            QStringLiteral("angry")) ||
            expression.contains(QStringLiteral("脸黑")) || expression.contains(QStringLiteral("生气")) ||
                                                                               emotion == QStringLiteral("angry");

        setParameter("ParamAngleX", gazeX * 18.0f + std::sin(idle * 0.8f) * 3.0f);
        setParameter("ParamAngleY", gazeY * 12.0f + std::sin(idle * 0.9f) * 2.0f);
        setParameter("ParamAngleZ", -gazeX * 7.0f + std::sin(idle * 0.55f) * 2.0f);
        setParameter("ParamBodyAngleX", gazeX * 8.0f);
        setParameter("ParamBodyAngleY", gazeY * 4.0f);
        setParameter("ParamBodyAngleZ", -gazeX * 4.0f + std::sin(idle * 0.7f) * 1.4f);
        setParameter("ParamBreath", breathe);
        setParameter("ParamEyeBallX", gazeX);
        setParameter("ParamEyeBallY", gazeY);
        setParameter("ParamEyeLOpen", blink);
        setParameter("ParamEyeROpen", blink);
        setParameter("ParamMouthForm", cheerful ? 0.8f : (angry ? -0.55f : 0.18f));

        const QString motion = state.motion.toLower();
        const float speakingPulse = (motion == QStringLiteral("talk") || motion == QStringLiteral("speak"))
                                     ? (0.5f + 0.5f * std::sin(idle * 16.0f)) * 0.85f
                                     : 0.0f;
        setParameter("ParamMouthOpenY", clamped(static_cast<float>(state.lipSyncValue) + speakingPulse, 0.0f, 1.0f));
        setParameter("ParamCheek", cheerful ? 1.0f : 0.0f);
        setParameter("lianhong", cheerful ? 1.0f : 0.0f);
        setParameter("shrngqi", angry ? 1.0f : 0.0f);
    }

    void resetSpecialExpressionParameters()
    {
        setParameter("aixinyan", 0.0f);
        setParameter("lianhong", 0.0f);
        setParameter("shrngqi", 0.0f);
        setParameter("tibi", 0.0f);
        setParameter("ParamCheek", 0.0f);
    }

    void setParameter(const char* id, float value, float weight = 1.0f)
    {
        if (!model || !id)
        {
            return;
        }
        const Csm::CubismIdHandle handle = Csm::CubismFramework::GetIdManager()->GetId(id);
        if (!handle)
        {
            return;
        }
        model->SetParameterValue(handle, value, weight);
    }

    QRectF modelBounds(bool visibleOnly) const
    {
        bool hasPoint = false;
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;

        const int drawableCount = model->GetDrawableCount();
        for (int drawable = 0; drawable < drawableCount; ++drawable)
        {
            if (visibleOnly && !model->GetDrawableDynamicFlagIsVisible(drawable))
            {
                continue;
            }
            const int vertexCount = model->GetDrawableVertexCount(drawable);
            const Live2D::Cubism::Core::csmVector2* positions = model->GetDrawableVertexPositions(drawable);
            if (!positions)
            {
                continue;
            }
            for (int i = 0; i < vertexCount; ++i)
            {
                if (!hasPoint)
                {
                    minX = maxX = positions[i].X;
                    minY = maxY = positions[i].Y;
                    hasPoint = true;
                }
                else
                {
                    minX = std::min(minX, positions[i].X);
                    minY = std::min(minY, positions[i].Y);
                    maxX = std::max(maxX, positions[i].X);
                    maxY = std::max(maxY, positions[i].Y);
                }
            }
        }

        if (!hasPoint)
        {
            return {};
        }
        return QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).normalized();
    }

    Csm::CubismMatrix44 fittedMvp(const QSize& viewportSize) const
    {
        QRectF bounds = modelBounds(true);
        if (bounds.isEmpty())
        {
            bounds = modelBounds(false);
        }
        if (bounds.isEmpty())
        {
            const float canvasWidth = std::max(1.0f, model->GetCanvasWidth());
            const float canvasHeight = std::max(1.0f, model->GetCanvasHeight());
            bounds = QRectF(-canvasWidth * 0.5f, -canvasHeight * 0.5f, canvasWidth, canvasHeight);
        }

        const float viewportWidth = static_cast<float>(viewportSize.width());
        const float viewportHeight = static_cast<float>(viewportSize.height());
        const float pixelScale = std::min((viewportWidth * 0.94f) / static_cast<float>(bounds.width()),
                                          (viewportHeight * 0.94f) / static_cast<float>(bounds.height()));
        const float scaleX = (2.0f * pixelScale) / viewportWidth;
        const float scaleY = (2.0f * pixelScale) / viewportHeight;
        const float translateX = -static_cast<float>(bounds.center().x()) * scaleX;
        const float translateY = -static_cast<float>(bounds.center().y()) * scaleY;

        Csm::CubismMatrix44 matrix;
        matrix.LoadIdentity();
        matrix.Scale(scaleX, scaleY);
        matrix.Translate(translateX, translateY);
        return matrix;
    }

    bool ready = false;
    QString error;
    QString modelPath;
    QString motionDirectory;
    QString expressionDirectory;
    Csm::CubismMoc* moc = nullptr;
    Csm::CubismModel* model = nullptr;
    CsmRendering::CubismRenderer_OpenGLES2* renderer = nullptr;
    Csm::CubismPhysics* physics = nullptr;
    Csm::CubismMotionManager* motionManager = nullptr;
    Csm::CubismExpressionMotionManager* expressionManager = nullptr;
    QHash<QString, Csm::ACubismMotion*> motions;
    QHash<QString, QString> motionAliases;
    QHash<Csm::ACubismMotion*, bool> motionLoopDefaults;
    QHash<QString, Csm::ACubismMotion*> expressions;
    QHash<QString, QString> expressionAliases;
    QHash<Csm::ACubismMotion*, bool> expressionLoopDefaults;
    Csm::ACubismMotion* currentMotion = nullptr;
    QString currentMotionKey;
    bool currentMotionPersistent = false;
    Csm::ACubismMotion* currentExpression = nullptr;
    bool currentExpressionPersistent = false;
    QVector<GLuint> textureIds;
    int lastActionSerial = -1;
    int lastExpressionSerial = -1;
    float lastIdlePhase = 0.0f;
    bool hasLastIdlePhase = false;
#else
    bool load(const QString&, const QString&, const QString&)
    {
        ready = false;
        error = QStringLiteral("Live2D Cubism Native OpenGL is not enabled in this build");
        return false;
    }

    bool render(const QSize&, const Live2DVisualState&)
    {
        return false;
    }

    bool ready = false;
    QString error = QStringLiteral("Live2D Cubism Native OpenGL is not enabled in this build");
#endif
};

Live2DOfficialOpenGLRenderer::Live2DOfficialOpenGLRenderer(const QString& modelPath,
                                                           const QString& motionDirectory,
                                                           const QString& expressionDirectory)
    : _impl(std::make_unique<Impl>())
{
    _impl->load(modelPath, motionDirectory, expressionDirectory);
}

Live2DOfficialOpenGLRenderer::~Live2DOfficialOpenGLRenderer() = default;

bool Live2DOfficialOpenGLRenderer::isReady() const
{
    return _impl && _impl->ready;
}

QString Live2DOfficialOpenGLRenderer::errorString() const
{
    return _impl ? _impl->error : QStringLiteral("Live2D renderer is not initialized");
}

QString Live2DOfficialOpenGLRenderer::rendererName() const
{
    return QStringLiteral("cubism-official-opengl");
}

bool Live2DOfficialOpenGLRenderer::isNative() const
{
    return isReady();
}

bool Live2DOfficialOpenGLRenderer::render(const QSize& viewportSize, const Live2DVisualState& state)
{
    return _impl && _impl->render(viewportSize, state);
}

QString Live2DOfficialOpenGLRenderer::defaultModelPath()
{
    return clientSourcePath(QStringLiteral("src/KafuuChino/香风智乃live2D/香风智乃.model3.json"));
}
