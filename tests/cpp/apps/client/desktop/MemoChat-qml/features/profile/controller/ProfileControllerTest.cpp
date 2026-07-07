#include "ProfileController.h"
#include "ProfileRequestPayloads.h"

#include <gtest/gtest.h>

#include <QJsonObject>
#include <QString>

TEST(ProfileControllerTest, StatusApiOwnsVisibleStatusAndSuppressesDuplicateSignals)
{
    ProfileController controller(nullptr);
    int statusChangedCount = 0;
    QObject::connect(&controller,
                     &ProfileController::statusChanged,
                     [&statusChangedCount]()
                     {
                         ++statusChangedCount;
                     });

    EXPECT_TRUE(controller.statusText().isEmpty());
    EXPECT_FALSE(controller.statusError());

    controller.setStatus(QStringLiteral("保存失败"), true);
    EXPECT_EQ(controller.statusText(), QStringLiteral("保存失败"));
    EXPECT_TRUE(controller.statusError());
    EXPECT_EQ(statusChangedCount, 1);

    controller.setStatus(QStringLiteral("保存失败"), true);
    EXPECT_EQ(statusChangedCount, 1);

    controller.syncStatus(QStringLiteral("保存成功"), false);
    EXPECT_EQ(controller.statusText(), QStringLiteral("保存成功"));
    EXPECT_FALSE(controller.statusError());
    EXPECT_EQ(statusChangedCount, 2);

    controller.clearStatusState();
    EXPECT_TRUE(controller.statusText().isEmpty());
    EXPECT_FALSE(controller.statusError());
    EXPECT_EQ(statusChangedCount, 3);
}

TEST(ProfileControllerTest, ClearStatusInvokableClearsFeatureState)
{
    ProfileController controller(nullptr);
    int statusChangedCount = 0;
    QObject::connect(&controller,
                     &ProfileController::statusChanged,
                     [&statusChangedCount]()
                     {
                         ++statusChangedCount;
                     });

    controller.setStatus(QStringLiteral("busy"), true);
    controller.clearStatus();

    EXPECT_TRUE(controller.statusText().isEmpty());
    EXPECT_FALSE(controller.statusError());
    EXPECT_EQ(statusChangedCount, 2);
}

TEST(ProfileControllerTest, ValidateProfileRejectsInvalidVisibleInputs)
{
    ProfileController controller(nullptr);
    QString errorText;

    EXPECT_FALSE(controller.validateProfile(QStringLiteral("   "), QStringLiteral("签名"), &errorText));
    EXPECT_EQ(errorText, QStringLiteral("昵称不能为空"));

    errorText.clear();
    EXPECT_FALSE(controller.validateProfile(QString(33, QChar('n')), QStringLiteral("签名"), &errorText));
    EXPECT_EQ(errorText, QStringLiteral("昵称长度不能超过32"));

    errorText.clear();
    EXPECT_FALSE(controller.validateProfile(QStringLiteral("nick"), QString(201, QChar('d')), &errorText));
    EXPECT_EQ(errorText, QStringLiteral("签名长度不能超过200"));

    errorText.clear();
    EXPECT_TRUE(controller.validateProfile(QStringLiteral(" nick "), QStringLiteral(" desc "), &errorText));
    EXPECT_TRUE(errorText.isEmpty());
}

TEST(ProfileControllerTest, SavePayloadTrimsEditableFieldsAndOmitsClientIdentityFields)
{
    const QJsonObject payload = memochat::profile_payload::buildSaveProfilePayload(
        QStringLiteral("memo_user"), QStringLiteral(" Nick "), QStringLiteral(" Desc "), QStringLiteral("avatar.png"));

    EXPECT_FALSE(payload.contains(QStringLiteral("uid")));
    EXPECT_FALSE(payload.contains(QStringLiteral("token")));
    EXPECT_EQ(payload.value(QStringLiteral("name")).toString(), QStringLiteral("memo_user"));
    EXPECT_EQ(payload.value(QStringLiteral("nick")).toString(), QStringLiteral("Nick"));
    EXPECT_EQ(payload.value(QStringLiteral("desc")).toString(), QStringLiteral("Desc"));
    EXPECT_EQ(payload.value(QStringLiteral("icon")).toString(), QStringLiteral("avatar.png"));
}

TEST(ProfileControllerTest, SettingsResponseParsesProfileAndSyncsThroughStatePort)
{
    ProfileController controller(nullptr);
    ProfileCurrentUserState snapshot;
    snapshot.uid = 42;
    snapshot.name = QStringLiteral("memo");
    snapshot.nick = QStringLiteral("old nick");
    snapshot.icon = QStringLiteral("qrc:/res/head_1.png");
    snapshot.desc = QStringLiteral("old desc");
    snapshot.userId = QStringLiteral("u123456789");
    snapshot.sex = 1;

    int syncCount = 0;
    ProfileAppliedUserState synced;
    controller.setStatePort(ProfileStatePort{[&snapshot]()
                                             {
                                                 return snapshot;
                                             },
                                             [&syncCount, &synced](const ProfileAppliedUserState& user)
                                             {
                                                 ++syncCount;
                                                 synced = user;
                                             }});

    controller.handleSettingsHttpFinished(
        ReqId::ID_UPDATE_PROFILE,
        QStringLiteral(R"({"error":0,"nick":"new nick","desc":"new desc","icon":"res/head_2.png"})"),
                       ErrorCodes::SUCCESS);

    EXPECT_EQ(syncCount, 1);
    EXPECT_EQ(synced.uid, 42);
    EXPECT_EQ(synced.name, QStringLiteral("memo"));
    EXPECT_EQ(synced.nick, QStringLiteral("new nick"));
    EXPECT_EQ(synced.desc, QStringLiteral("new desc"));
    EXPECT_EQ(synced.icon, QStringLiteral("qrc:/res/head_2.png"));
    EXPECT_EQ(synced.userId, QStringLiteral("u123456789"));
    EXPECT_EQ(synced.sex, 1);
    EXPECT_EQ(controller.statusText(), QStringLiteral("资料已同步"));
    EXPECT_FALSE(controller.statusError());
}

TEST(ProfileControllerTest, SettingsResponseOwnsErrorStatusWithoutSyncingUser)
{
    ProfileController controller(nullptr);
    int syncCount = 0;
    controller.setStatePort(ProfileStatePort{[]
                                             {
                                                 return ProfileCurrentUserState{};
                                             },
                                             [&syncCount](const ProfileAppliedUserState&)
                                             {
                                                 ++syncCount;
                                             }});

    controller.handleSettingsHttpFinished(ReqId::ID_UPDATE_PROFILE, QString(), ErrorCodes::ERR_NETWORK);
    EXPECT_EQ(syncCount, 0);
    EXPECT_EQ(controller.statusText(), QStringLiteral("资料同步失败：网络错误"));
    EXPECT_TRUE(controller.statusError());

    controller.handleSettingsHttpFinished(ReqId::ID_UPDATE_PROFILE, QStringLiteral("{"), ErrorCodes::SUCCESS);
    EXPECT_EQ(syncCount, 0);
    EXPECT_EQ(controller.statusText(), QStringLiteral("资料同步失败：响应解析错误"));
    EXPECT_TRUE(controller.statusError());

    controller.handleSettingsHttpFinished(ReqId::ID_UPDATE_PROFILE,
                                          QStringLiteral(R"({"error":2})"), ErrorCodes::SUCCESS);
    EXPECT_EQ(syncCount, 0);
    EXPECT_EQ(controller.statusText(), QStringLiteral("资料同步失败"));
    EXPECT_TRUE(controller.statusError());
}

TEST(ProfileControllerTest, ApplyCurrentUserProfilePreservesExistingIconWhenRequested)
{
    ProfileController controller(nullptr);
    ProfileCurrentUserState snapshot;
    snapshot.pendingUid = 77;
    snapshot.icon = QStringLiteral("qrc:/res/custom.png");

    ProfileAppliedUserState synced;
    controller.setStatePort(ProfileStatePort{[&snapshot]()
                                             {
                                                 return snapshot;
                                             },
                                             [&synced](const ProfileAppliedUserState& user)
                                             {
                                                 synced = user;
                                             }});

    QJsonObject profile;
    profile.insert(QStringLiteral("name"), QStringLiteral("memo"));
    profile.insert(QStringLiteral("nick"), QStringLiteral("nick"));
    profile.insert(QStringLiteral("icon"), QStringLiteral("qrc:/res/head_1.png"));
    profile.insert(QStringLiteral("desc"), QStringLiteral("desc"));
    profile.insert(QStringLiteral("user_id"), QStringLiteral("u987654321"));
    profile.insert(QStringLiteral("sex"), 2);

    controller.applyCurrentUserProfile(profile, true);

    EXPECT_EQ(synced.uid, 77);
    EXPECT_EQ(synced.name, QStringLiteral("memo"));
    EXPECT_EQ(synced.nick, QStringLiteral("nick"));
    EXPECT_EQ(synced.icon, QStringLiteral("qrc:/res/custom.png"));
    EXPECT_EQ(synced.desc, QStringLiteral("desc"));
    EXPECT_EQ(synced.userId, QStringLiteral("u987654321"));
    EXPECT_EQ(synced.sex, 2);
}

TEST(ProfileControllerTest, SendSaveProfileAllowsMissingGateway)
{
    ProfileController controller(nullptr);

    EXPECT_NO_THROW(controller.sendSaveProfile(
        7,
        QStringLiteral("access"),
                       QStringLiteral("name"), QStringLiteral("nick"), QStringLiteral("desc"), QStringLiteral("icon")));
}
