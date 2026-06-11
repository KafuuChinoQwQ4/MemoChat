#include "AuthViewModel.h"

#include <gtest/gtest.h>

#include <QString>
#include <utility>

namespace
{
void clearCredentialCache()
{
    QSettings settings = AuthCredentialStoreDetail::makeAuthSettings();
    settings.beginGroup(QString::fromLatin1(AuthCredentialStoreDetail::kLoginCredentialSettingsGroup));
    settings.remove(QString());
    settings.endGroup();
    settings.sync();
}

class CredentialCacheGuard
{
public:
    CredentialCacheGuard()
    {
        clearCredentialCache();
    }

    ~CredentialCacheGuard()
    {
        clearCredentialCache();
    }
};
} // namespace

TEST(AuthViewModelTest, RegisterCountdownAndCooldownAreFeatureOwnedAndSignalOnChanges)
{
    AuthViewModel viewModel(nullptr);
    int stateChangedCount = 0;
    QObject::connect(&viewModel,
                     &AuthViewModel::stateChanged,
                     [&stateChangedCount]()
                     {
                         ++stateChangedCount;
                     });

    EXPECT_EQ(viewModel.registerCountdown(), 5);
    EXPECT_EQ(viewModel.registerCodeCooldownSeconds(), 0);
    EXPECT_FALSE(viewModel.registerCodeRequestPending());

    viewModel.syncRegisterCountdown(4);
    EXPECT_EQ(viewModel.registerCountdown(), 4);
    EXPECT_EQ(stateChangedCount, 1);
    viewModel.syncRegisterCountdown(4);
    EXPECT_EQ(stateChangedCount, 1);

    viewModel.syncRegisterCodeCooldownSeconds(60);
    EXPECT_EQ(viewModel.registerCodeCooldownSeconds(), 60);
    EXPECT_EQ(stateChangedCount, 2);
    viewModel.syncRegisterCodeCooldownSeconds(-10);
    EXPECT_EQ(viewModel.registerCodeCooldownSeconds(), 0);
    EXPECT_EQ(stateChangedCount, 3);

    viewModel.syncRegisterCodeRequestPending(true);
    EXPECT_TRUE(viewModel.registerCodeRequestPending());
    EXPECT_EQ(stateChangedCount, 4);
    viewModel.syncRegisterCodeRequestPending(true);
    EXPECT_EQ(stateChangedCount, 4);
}

TEST(AuthViewModelTest, LoginCredentialCacheJsonIsFeatureOwnedAndHasDedicatedSignal)
{
    CredentialCacheGuard credentialGuard;
    AuthViewModel viewModel(nullptr);
    int stateChangedCount = 0;
    int cacheChangedCount = 0;
    QObject::connect(&viewModel,
                     &AuthViewModel::stateChanged,
                     [&stateChangedCount]()
                     {
                         ++stateChangedCount;
                     });
    QObject::connect(&viewModel,
                     &AuthViewModel::loginCredentialCacheChanged,
                     [&cacheChangedCount]()
                     {
                         ++cacheChangedCount;
                     });

    EXPECT_EQ(viewModel.loginCredentialCacheJson(), QStringLiteral("[]"));

    viewModel.syncLoginCredentialCacheJson(QStringLiteral("  "));
    EXPECT_EQ(viewModel.loginCredentialCacheJson(), QStringLiteral("[]"));
    EXPECT_EQ(cacheChangedCount, 0);

    const QString cache = QStringLiteral(R"([{"email":"a@example.com"}])");
    viewModel.syncLoginCredentialCacheJson(cache);
    EXPECT_EQ(viewModel.loginCredentialCacheJson(), cache);
    EXPECT_EQ(cacheChangedCount, 1);
    EXPECT_EQ(stateChangedCount, 0);

    viewModel.syncLoginCredentialCacheJson(cache);
    EXPECT_EQ(cacheChangedCount, 1);
}

TEST(AuthViewModelTest, TipBusyAndSuccessStateRemainSelfContained)
{
    AuthViewModel viewModel(nullptr);
    int stateChangedCount = 0;
    QObject::connect(&viewModel,
                     &AuthViewModel::stateChanged,
                     [&stateChangedCount]()
                     {
                         ++stateChangedCount;
                     });

    viewModel.syncTip(QStringLiteral("邮箱无效"), true);
    EXPECT_EQ(viewModel.tipText(), QStringLiteral("邮箱无效"));
    EXPECT_TRUE(viewModel.tipError());
    EXPECT_EQ(stateChangedCount, 1);

    viewModel.syncBusy(true);
    EXPECT_TRUE(viewModel.busy());
    EXPECT_EQ(stateChangedCount, 2);

    viewModel.syncRegisterSuccessPage(true);
    EXPECT_TRUE(viewModel.registerSuccessPage());
    EXPECT_EQ(stateChangedCount, 3);

    viewModel.syncRegisterSuccessPage(true);
    EXPECT_EQ(stateChangedCount, 3);
}

TEST(AuthViewModelTest, InvokableCommandsUseCommandPortAndLocalCredentialStore)
{
    CredentialCacheGuard credentialGuard;
    AuthViewModel viewModel(nullptr);
    int cacheChangedCount = 0;
    int clearTipCount = 0;
    QString savedEmail;
    QString loginEmail;
    QString loginPassword;
    QString codeEmail;
    QString resetCodeEmail;
    QString registeredUser;
    QString resetUser;

    QObject::connect(&viewModel,
                     &AuthViewModel::loginCredentialCacheChanged,
                     [&cacheChangedCount]()
                     {
                         ++cacheChangedCount;
                     });

    AuthCommandPort commandPort;
    commandPort.clearTip = [&clearTipCount]()
    {
        ++clearTipCount;
    };
    commandPort.login = [&loginEmail, &loginPassword](const QString& email, const QString& password)
    {
        loginEmail = email;
        loginPassword = password;
    };
    commandPort.requestRegisterCode = [&codeEmail](const QString& email)
    {
        codeEmail = email;
    };
    commandPort.registerUser = [&registeredUser](const QString& user,
                                                 const QString& email,
                                                 const QString& password,
                                                 const QString& confirm,
                                                 const QString& verifyCode)
    {
        Q_UNUSED(email)
        Q_UNUSED(password)
        Q_UNUSED(confirm)
        Q_UNUSED(verifyCode)
        registeredUser = user;
    };
    commandPort.requestResetCode = [&resetCodeEmail](const QString& email)
    {
        resetCodeEmail = email;
    };
    commandPort.resetPassword =
        [&resetUser](const QString& user, const QString& email, const QString& password, const QString& verifyCode)
    {
        Q_UNUSED(email)
        Q_UNUSED(password)
        Q_UNUSED(verifyCode)
        resetUser = user;
    };
    viewModel.setCommandPort(std::move(commandPort));

    viewModel.clearTip();
    viewModel.saveLoginCredential(QStringLiteral(" saved@example.com "), QStringLiteral("pw"));
    viewModel.login(QStringLiteral("login@example.com"), QStringLiteral("login-pw"));
    viewModel.requestRegisterCode(QStringLiteral("code@example.com"));
    viewModel.registerUser(QStringLiteral("new_user"),
        QStringLiteral("new@example.com"), QStringLiteral("pw"), QStringLiteral("pw"), QStringLiteral("123456"));
    viewModel.requestResetCode(QStringLiteral("reset-code@example.com"));
    viewModel.resetPassword(QStringLiteral("reset_user"),
                                           QStringLiteral("reset@example.com"),
                                                          QStringLiteral("pw"), QStringLiteral("654321"));

    EXPECT_EQ(clearTipCount, 1);
    savedEmail = viewModel.loginCredentialCacheJson();
    EXPECT_TRUE(savedEmail.contains(QStringLiteral("saved@example.com")));
    EXPECT_TRUE(savedEmail.contains(QStringLiteral("login@example.com")));
    EXPECT_GE(cacheChangedCount, 2);
    EXPECT_EQ(loginEmail, QStringLiteral("login@example.com"));
    EXPECT_EQ(loginPassword, QStringLiteral("login-pw"));
    EXPECT_EQ(codeEmail, QStringLiteral("code@example.com"));
    EXPECT_EQ(resetCodeEmail, QStringLiteral("reset-code@example.com"));
    EXPECT_EQ(registeredUser, QStringLiteral("new_user"));
    EXPECT_EQ(resetUser, QStringLiteral("reset_user"));
}
