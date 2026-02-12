#pragma once
#include <QDialog>
#include "global.h"
#include "ChatUserList.h"
#include "LoadingDlg.h"
#include "ChatPage.h"
#include "StateWidget.h" // [Changed] 引入 StateWidget
#include "SearchList.h"  // [New] 引入 SearchList
#include <QMouseEvent>
#include "ContactUserList.h" // [New]
#include "ApplyFriendPage.h"

class QStackedWidget;
class CustomizeEdit;

class ChatDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();

private slots:
    void slot_loading_chat_user();
    // [New] 侧边栏和搜索槽函数
    void slot_side_chat();
    void slot_side_contact();
    void slot_text_changed(const QString &str);
    void slot_switch_apply_friend_page();
    void slot_apply_friend(std::shared_ptr<AddFriendApply> apply);
    void slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info);
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);
    void slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void initUI(); 
    void addSideBar(); 
    void addChatList(); 
    void addChatBox(); 
    void addChatUserList(); 
    
    // [New] 状态管理函数
    void ClearLabelState(StateWidget *lb);
    void AddLBGroup(StateWidget *lb);
    void ShowSearch(bool bsearch);
    void handleGlobalMousePress(QMouseEvent *event);
    void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata);

private:
    QWidget *_side_bar;
    QWidget *_chat_list_wid;
    CustomizeEdit *_search_edit;
    
    ChatUserList *_chat_list;
    SearchList *_search_list; // [New] 搜索列表
    
    QStackedWidget *_stacked_widget;
    ChatPage *_chat_page;

    // 侧边栏按钮 [Changed]
    StateWidget *_side_chat_btn;
    StateWidget *_side_contact_btn;
    QList<StateWidget*> _lb_list; // 管理按钮组

    bool _b_loading;

    ContactUserList *_contact_list; // [New]
    ApplyFriendPage *_apply_friend_page;
    
    QMap<int, QListWidgetItem*> _chat_items_added;
    // UI 模式枚举
    enum ChatUIMode {
        ChatMode,
        ContactMode,
        SearchMode
    };
    ChatUIMode _state;
};