#ifndef SHELLVIEWMODEL_H
#define SHELLVIEWMODEL_H

#include <QObject>
#include <QString>

class ShellViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int page READ page NOTIFY pageChanged)
    Q_PROPERTY(int chatTab READ chatTab NOTIFY chatTabChanged)
    Q_PROPERTY(QString currentUserName READ currentUserName NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserNick READ currentUserNick NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserIcon READ currentUserIcon NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserDesc READ currentUserDesc NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserId READ currentUserId NOTIFY currentUserChanged)
    Q_PROPERTY(int currentUserUid READ currentUserUid NOTIFY currentUserChanged)

public:
    enum Page
    {
        LoginPage = 0,
        RegisterPage = 1,
        ResetPage = 2,
        ChatPage = 3
    };
    Q_ENUM(Page)

    enum ChatTab
    {
        ChatTabPage = 0,
        ContactTabPage = 1,
        SettingsTabPage = 2,
        MomentsTabPage = 3,
        AgentTabPage = 4,
        Live2DTabPage = 5
    };
    Q_ENUM(ChatTab)

    enum ContactPane
    {
        ApplyRequestPane = 0,
        FriendInfoPane = 1
    };
    Q_ENUM(ContactPane)

    explicit ShellViewModel(QObject* parent = nullptr);

    int page() const;
    int chatTab() const;
    QString currentUserName() const;
    QString currentUserNick() const;
    QString currentUserIcon() const;
    QString currentUserDesc() const;
    QString currentUserId() const;
    int currentUserUid() const;

    Q_INVOKABLE void switchToLogin();
    Q_INVOKABLE void switchToRegister();
    Q_INVOKABLE void switchToReset();
    Q_INVOKABLE void switchChatTab(int tab);
    Q_INVOKABLE void beginPostLoginBootstrap();
    Q_INVOKABLE void openExternalResource(const QString& url);

    void syncPage(int page);
    void syncChatTab(int chatTab);
    void syncCurrentUser(const QString& name,
                         const QString& nick,
                         const QString& icon,
                         const QString& desc,
                         const QString& userId,
                         int uid);

signals:
    void pageChanged();
    void chatTabChanged();
    void currentUserChanged();

    void switchToLoginRequested();
    void switchToRegisterRequested();
    void switchToResetRequested();
    void switchChatTabRequested(int tab);
    void beginPostLoginBootstrapRequested();
    void openExternalResourceRequested(const QString& url);

private:
    int _page = 0;
    int _chat_tab = 0;
    QString _current_user_name;
    QString _current_user_nick;
    QString _current_user_icon = QStringLiteral("qrc:/res/head_1.png");
    QString _current_user_desc;
    QString _current_user_id;
    int _current_user_uid = 0;
};

#endif // SHELLVIEWMODEL_H
