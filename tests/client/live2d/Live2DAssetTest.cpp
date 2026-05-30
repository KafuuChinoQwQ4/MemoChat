#include "Live2DAsset.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
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
