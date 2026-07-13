#include "Live2DAsset.h"
#include "Live2DAssetActionFiles.h"
#include "Live2DAssetModelFile.h"
#include "Live2DAssetOptionalFiles.h"
#include "Live2DAssetPaths.h"
#include "Live2DAssetRequiredFiles.h"
#include "Live2DAssetValidation.h"
#include "Live2DAssetVoiceFiles.h"
#include "Live2DModelAssetParser.h"
#include "Live2DMotionCatalog.h"
#include "Live2DRenderPathResolver.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QTemporaryDir>
#include <QUrl>
#include <gtest/gtest.h>

namespace
{

QString fixtureRoot()
{
    return QString::fromLocal8Bit(LIVE2D_FIXTURE_DIR);
}

void writeFile(const QString& path, const QByteArray& content)
{
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(content), content.size());
}

} // namespace

TEST(Live2DAssetTest, ValidatesMinimalFixturePackage)
{
    const QString root = fixtureRoot() + QStringLiteral("/minimal_model");

    Live2DAsset asset;
    asset.setModelRoot(root);
    asset.setModelJson(root + QStringLiteral("/minimal.model3.json"));
    asset.setMotionDirectory(root + QStringLiteral("/motions"));
    asset.setExpressionDirectory(root + QStringLiteral("/expressions"));
    asset.setVoiceDirectory(root + QStringLiteral("/voice"));
    asset.validate();

    EXPECT_TRUE(asset.valid());
    EXPECT_EQ(asset.status(), QStringLiteral("ready"));
    EXPECT_TRUE(asset.errors().isEmpty());
    EXPECT_EQ(asset.motionCount(), 1);
    EXPECT_EQ(asset.expressionCount(), 1);
    EXPECT_EQ(asset.textureCount(), 1);
    EXPECT_EQ(asset.voiceCount(), 1);
    EXPECT_FALSE(asset.physicsFile().isEmpty());
    EXPECT_FALSE(asset.poseFile().isEmpty());
    EXPECT_FALSE(asset.userDataFile().isEmpty());
    EXPECT_FALSE(asset.vtubeMappingFile().isEmpty());
    EXPECT_EQ(asset.packageChecksum().size(), 64);
    EXPECT_GE(asset.referencedFileCount(), 8);

    const QVariantList actions = asset.actionItems();
    ASSERT_EQ(actions.size(), 2);
    const QVariantMap firstAction = actions.at(0).toMap();
    const QVariantMap secondAction = actions.at(1).toMap();
    EXPECT_EQ(firstAction.value(QStringLiteral("kind")).toString(), QStringLiteral("motion"));
    EXPECT_TRUE(
        firstAction.value(QStringLiteral("file")).toString().endsWith(QStringLiteral("minimal_idle.motion3.json")));
    EXPECT_EQ(secondAction.value(QStringLiteral("kind")).toString(), QStringLiteral("expression"));
    EXPECT_TRUE(
        secondAction.value(QStringLiteral("file")).toString().endsWith(QStringLiteral("minimal_smile.exp3.json")));
    EXPECT_TRUE(asset.statusText().contains(QStringLiteral("1 动作 / 1 表情 / 1 贴图 / 1 语音")));
}

TEST(Live2DAssetTest, ValidatorMatchesWrapperForMinimalFixturePackage)
{
    const QString root = fixtureRoot() + QStringLiteral("/minimal_model");
    Live2DAssetValidation::Inputs inputs;
    inputs.modelRoot = root;
    inputs.modelJson = root + QStringLiteral("/minimal.model3.json");
    inputs.motionDirectory = root + QStringLiteral("/motions");
    inputs.expressionDirectory = root + QStringLiteral("/expressions");
    inputs.voiceDirectory = root + QStringLiteral("/voice");

    Live2DAsset asset;
    asset.setModelRoot(inputs.modelRoot);
    asset.setModelJson(inputs.modelJson);
    asset.setMotionDirectory(inputs.motionDirectory);
    asset.setExpressionDirectory(inputs.expressionDirectory);
    asset.setVoiceDirectory(inputs.voiceDirectory);
    asset.validate();

    const Live2DAssetValidation::Result result = Live2DAssetValidation::validate(inputs);

    EXPECT_EQ(result.status, asset.status());
    EXPECT_EQ(result.statusText, asset.statusText());
    EXPECT_EQ(result.errors, asset.errors());
    EXPECT_EQ(result.warnings, asset.warnings());
    EXPECT_EQ(result.motionCount, asset.motionCount());
    EXPECT_EQ(result.expressionCount, asset.expressionCount());
    EXPECT_EQ(result.textureCount, asset.textureCount());
    EXPECT_EQ(result.voiceCount, asset.voiceCount());
    EXPECT_EQ(result.packageChecksum.size(), asset.packageChecksum().size());
    EXPECT_EQ(result.referencedFileCount, asset.referencedFileCount());
    EXPECT_EQ(result.vtubeMappingFile, asset.vtubeMappingFile());
    EXPECT_EQ(result.actionItems.size(), asset.actionItems().size());
}

TEST(Live2DAssetTest, EmptyModelJsonIsNonCrashingEmptyState)
{
    Live2DAsset asset;
    asset.validate();

    EXPECT_FALSE(asset.valid());
    EXPECT_EQ(asset.status(), QStringLiteral("empty"));
    EXPECT_TRUE(asset.errors().isEmpty());
    EXPECT_TRUE(asset.packageChecksum().isEmpty());
    EXPECT_EQ(asset.referencedFileCount(), 0);
}

TEST(Live2DAssetTest, MissingRequiredTextureIsInvalid)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    const QString modelPath = temp.filePath(QStringLiteral("broken.model3.json"));
    writeFile(temp.filePath(QStringLiteral("model.moc3")), QByteArray("moc"));
    writeFile(modelPath, R"json({
        "Version": 3,
        "FileReferences": {
            "Moc": "model.moc3",
            "Textures": ["textures/missing.png"]
        }
    })json");

    Live2DAsset asset;
    asset.setModelRoot(temp.path());
    asset.setModelJson(modelPath);
    asset.validate();

    EXPECT_FALSE(asset.valid());
    EXPECT_EQ(asset.status(), QStringLiteral("invalid"));
    EXPECT_TRUE(asset.packageChecksum().isEmpty());
    ASSERT_FALSE(asset.errors().isEmpty());
    EXPECT_TRUE(asset.errors().join(QStringLiteral("\n")).contains(QStringLiteral("贴图文件不存在")));
}

TEST(Live2DAssetTest, MalformedModelJsonIsInvalid)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    const QString modelPath = temp.filePath(QStringLiteral("bad.model3.json"));
    writeFile(modelPath, QByteArray("{ not-json"));

    Live2DAsset asset;
    asset.setModelRoot(temp.path());
    asset.setModelJson(modelPath);
    asset.validate();

    EXPECT_FALSE(asset.valid());
    EXPECT_EQ(asset.status(), QStringLiteral("invalid"));
    EXPECT_TRUE(asset.packageChecksum().isEmpty());
    ASSERT_FALSE(asset.errors().isEmpty());
    EXPECT_TRUE(asset.errors().join(QStringLiteral("\n")).contains(QStringLiteral("格式无效")));
}

TEST(Live2DAssetTest, ParserUtilitiesResolveFixturePackageFiles)
{
    const QString root = fixtureRoot() + QStringLiteral("/minimal_model");

    EXPECT_TRUE(Live2DModelAssetParser::fileExists(root + QStringLiteral("/textures/minimal_texture_00.png")));
    EXPECT_TRUE(Live2DModelAssetParser::fileExists(root + QStringLiteral("/minimal.physics3.json")));
    EXPECT_FALSE(Live2DModelAssetParser::fileExists(root + QStringLiteral("/textures/missing.png")));

    QStringList packageFiles;
    Live2DModelAssetParser::appendExistingPackageFile(packageFiles, root + QStringLiteral("/minimal.model3.json"));
    Live2DModelAssetParser::appendExistingPackageFile(packageFiles, root + QStringLiteral("/minimal.model3.json"));
    Live2DModelAssetParser::appendExistingPackageFile(packageFiles, root + QStringLiteral("/textures/missing.png"));
    EXPECT_EQ(packageFiles.size(), 1);
}

TEST(Live2DAssetTest, MotionCatalogDeduplicatesActionsByResolvedFile)
{
    const QString root = fixtureRoot() + QStringLiteral("/minimal_model");
    const QString motionPath = root + QStringLiteral("/motions/minimal_idle.motion3.json");
    const QString displayName = Live2DMotionCatalog::actionDisplayName(QString(), motionPath, QString());
    QVariantList actions;
    QSet<QString> keys;
    QSet<QString> files;

    Live2DMotionCatalog::appendActionItem(actions,
                                          keys,
                                          files,
                                          QStringLiteral("motion"),
                                                         displayName,
                                                         displayName,
                                                         QStringLiteral("Idle"), 0, motionPath, QStringLiteral("test"));
    Live2DMotionCatalog::appendActionItem(
        actions,
        keys,
        files,
        QStringLiteral("motion"),
                       QStringLiteral("duplicate"),
                                      QStringLiteral("duplicate"),
                                                     QStringLiteral("Idle"), 1, motionPath, QStringLiteral("test"));

    ASSERT_EQ(actions.size(), 1);
    const QVariantMap action = actions.first().toMap();
    EXPECT_EQ(action.value(QStringLiteral("kind")).toString(), QStringLiteral("motion"));
    EXPECT_EQ(action.value(QStringLiteral("name")).toString(), QStringLiteral("minimal_idle"));
    EXPECT_EQ(action.value(QStringLiteral("file")).toString(), motionPath);
}

TEST(Live2DAssetTest, AssetPathsResolveInputsAndCountFiles)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    writeFile(temp.filePath(QStringLiteral("voice.wav")), QByteArray("voice"));
    writeFile(temp.filePath(QStringLiteral("voice.mp3")), QByteArray("voice"));
    writeFile(temp.filePath(QStringLiteral("note.txt")), QByteArray("text"));

    const QString modelDirectory = QDir(temp.path()).absoluteFilePath(QStringLiteral("model"));
    ASSERT_TRUE(QDir().mkpath(modelDirectory));
    const QString modelPath = QDir(modelDirectory).absoluteFilePath(QStringLiteral("avatar.model3.json"));
    writeFile(modelPath, QByteArray("{}"));

    const QString fileUrl = QUrl::fromLocalFile(modelPath).toString();
    EXPECT_EQ(Live2DAssetPaths::resolveInputPath(fileUrl), QDir::cleanPath(modelPath));
    EXPECT_EQ(Live2DAssetPaths::resolveModelReference(
        modelDirectory,
        QStringLiteral("avatar.model3.json")),
        QDir(modelDirectory).absoluteFilePath(QStringLiteral("avatar.model3.json")));
    EXPECT_EQ(
        Live2DAssetPaths::countFilesWithSuffixes(temp.path(), {QStringLiteral(".wav"), QStringLiteral(".mp3")}), 2);

    const QJsonArray items{QStringLiteral(" idle "), QString(), QStringLiteral("smile")};
    EXPECT_EQ(Live2DAssetPaths::jsonStringArray(items), QStringList({QStringLiteral("idle"), QStringLiteral("smile")}));
}

TEST(Live2DAssetTest, VoiceFilesExposeSuffixesAndCollectPackageFiles)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    writeFile(temp.filePath(QStringLiteral("voice.wav")), QByteArray("voice"));
    writeFile(temp.filePath(QStringLiteral("voice.mp3")), QByteArray("voice"));
    writeFile(temp.filePath(QStringLiteral("voice.ogg")), QByteArray("voice"));
    writeFile(temp.filePath(QStringLiteral("voice.flac")), QByteArray("voice"));
    writeFile(temp.filePath(QStringLiteral("voice.m4a")), QByteArray("voice"));
    writeFile(temp.filePath(QStringLiteral("note.txt")), QByteArray("text"));

    EXPECT_EQ(Live2DAssetVoiceFiles::suffixes(),
              QStringList({QStringLiteral(".wav"),
                  QStringLiteral(".mp3"), QStringLiteral(".ogg"), QStringLiteral(".flac"), QStringLiteral(".m4a")}));
    EXPECT_EQ(
        Live2DAssetVoiceFiles::nameFilters(),
        QStringList({QStringLiteral("*.wav"),
            QStringLiteral("*.mp3"), QStringLiteral("*.ogg"), QStringLiteral("*.flac"), QStringLiteral("*.m4a")}));
    EXPECT_EQ(Live2DAssetVoiceFiles::count(temp.path()), 5);

    QStringList packageFiles;
    Live2DAssetVoiceFiles::appendPackageFiles(packageFiles, temp.path());

    EXPECT_EQ(packageFiles.size(), 5);
    EXPECT_TRUE(packageFiles.join(QStringLiteral("\n")).contains(QStringLiteral("voice.wav")));
    EXPECT_FALSE(packageFiles.join(QStringLiteral("\n")).contains(QStringLiteral("note.txt")));
}

TEST(Live2DAssetTest, OptionalFilesResolveFixtureReferencesAndCollectPackageFiles)
{
    const QString root = fixtureRoot() + QStringLiteral("/minimal_model");
    QFile modelFile(root + QStringLiteral("/minimal.model3.json"));
    ASSERT_TRUE(modelFile.open(QIODevice::ReadOnly));
    const QJsonObject refs =
        QJsonDocument::fromJson(modelFile.readAll()).object().value(QStringLiteral("FileReferences")).toObject();

    QStringList warnings;
    QStringList packageFiles;
    const Live2DAssetOptionalFiles::Result result =
        Live2DAssetOptionalFiles::collect(refs, root, QString(), warnings, packageFiles);

    EXPECT_TRUE(result.physicsFile.endsWith(QStringLiteral("minimal.physics3.json")));
    EXPECT_TRUE(result.poseFile.endsWith(QStringLiteral("minimal.pose3.json")));
    EXPECT_TRUE(result.userDataFile.endsWith(QStringLiteral("minimal.userdata3.json")));
    EXPECT_TRUE(result.vtubeMappingFile.endsWith(QStringLiteral("minimal.vtube.json")));
    EXPECT_TRUE(warnings.isEmpty());
    EXPECT_EQ(packageFiles.size(), 4);
}

TEST(Live2DAssetTest, RequiredFilesResolveFixtureMocAndTextures)
{
    const QString root = fixtureRoot() + QStringLiteral("/minimal_model");
    QFile modelFile(root + QStringLiteral("/minimal.model3.json"));
    ASSERT_TRUE(modelFile.open(QIODevice::ReadOnly));
    const QJsonObject refs =
        QJsonDocument::fromJson(modelFile.readAll()).object().value(QStringLiteral("FileReferences")).toObject();

    QStringList errors;
    QStringList warnings;
    QStringList packageFiles;
    const Live2DAssetRequiredFiles::Result result =
        Live2DAssetRequiredFiles::collect(refs, root, errors, warnings, packageFiles);

    EXPECT_EQ(result.textureCount, 1);
    EXPECT_TRUE(errors.isEmpty());
    EXPECT_TRUE(warnings.isEmpty());
    EXPECT_TRUE(packageFiles.join(QStringLiteral("\n")).contains(QStringLiteral("minimal.moc3")));
    EXPECT_TRUE(packageFiles.join(QStringLiteral("\n")).contains(QStringLiteral("minimal_texture_00.png")));
}

TEST(Live2DAssetTest, ModelFileLoadsFixtureReferencesAndPackageEntry)
{
    const QString root = fixtureRoot() + QStringLiteral("/minimal_model");

    QStringList errors;
    QStringList warnings;
    QStringList packageFiles;
    const Live2DAssetModelFile::Result result =
        Live2DAssetModelFile::load(root, root + QStringLiteral("/minimal.model3.json"), errors, warnings, packageFiles);

    EXPECT_TRUE(result.loaded);
    EXPECT_TRUE(result.terminalStatus.isEmpty());
    EXPECT_TRUE(result.terminalStatusText.isEmpty());
    EXPECT_TRUE(errors.isEmpty());
    EXPECT_TRUE(warnings.isEmpty());
    EXPECT_EQ(result.modelDirectory, root);
    EXPECT_EQ(result.refs.value(QStringLiteral("Moc")).toString(), QStringLiteral("minimal.moc3"));
    EXPECT_TRUE(result.refs.contains(QStringLiteral("Textures")));
    EXPECT_TRUE(packageFiles.join(QStringLiteral("\n")).contains(QStringLiteral("minimal.model3.json")));
}

TEST(Live2DAssetTest, RenderPathResolverRequiresUserSelectedModel)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    const QString modelPath = temp.filePath(QStringLiteral("avatar.model3.json"));
    writeFile(modelPath, QByteArray("{}"));

    EXPECT_TRUE(Live2DRenderPathResolver::resolveModelPath(temp.path(), QString()).isEmpty());
    EXPECT_EQ(Live2DRenderPathResolver::resolveModelPath(temp.path(), QStringLiteral("qrc:/models/a.model3.json")),
                                                         QStringLiteral(":/models/a.model3.json"));
    EXPECT_EQ(Live2DRenderPathResolver::resolveModelPath(temp.path(), QUrl::fromLocalFile(modelPath).toString()),
              QDir::cleanPath(modelPath));
    EXPECT_EQ(Live2DRenderPathResolver::resolveModelPath(temp.path(), QStringLiteral("avatar.model3.json")),
                                                         QDir::cleanPath(modelPath));
    EXPECT_EQ(Live2DRenderPathResolver::resolveModelPath(temp.path(), QStringLiteral("missing.model3.json")),
                                                         QDir::cleanPath(
                                                             temp.filePath(QStringLiteral("missing.model3.json"))));
}

TEST(Live2DAssetTest, ActionFilesCollectFixtureMotionsAndExpressions)
{
    const QString root = fixtureRoot() + QStringLiteral("/minimal_model");
    QFile modelFile(root + QStringLiteral("/minimal.model3.json"));
    ASSERT_TRUE(modelFile.open(QIODevice::ReadOnly));
    const QJsonObject refs =
        QJsonDocument::fromJson(modelFile.readAll()).object().value(QStringLiteral("FileReferences")).toObject();

    QStringList warnings;
    QStringList packageFiles;
    const Live2DAssetActionFiles::Result result = Live2DAssetActionFiles::collect(
        refs,
        root,
        root + QStringLiteral("/motions"), root + QStringLiteral("/expressions"), warnings, packageFiles);

    EXPECT_EQ(result.motionCount, 1);
    EXPECT_EQ(result.expressionCount, 1);
    ASSERT_EQ(result.actionItems.size(), 2);
    EXPECT_EQ(result.actionItems.at(0).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("motion"));
    EXPECT_EQ(result.actionItems.at(1).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("expression"));
    EXPECT_TRUE(packageFiles.join(QStringLiteral("\n")).contains(QStringLiteral("minimal_idle.motion3.json")));
    EXPECT_TRUE(packageFiles.join(QStringLiteral("\n")).contains(QStringLiteral("minimal_smile.exp3.json")));
    EXPECT_TRUE(warnings.isEmpty());
}
