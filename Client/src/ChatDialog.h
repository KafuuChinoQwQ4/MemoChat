#pragma once
#include <QDialog>
#include "global.h"
#include "ChatUserList.h" // 引入新列表头文件
#include "LoadingDlg.h"   // 引入加载弹窗
#include "ChatPage.h"     // 引入聊天页

class QStackedWidget;
class CustomizeEdit;

class ChatDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();

private slots:
    // [新增] 加载更多用户的槽函数
    void slot_loading_chat_user();

private:
    void initUI(); 
    void addSideBar(); 
    void addChatList(); 
    void addChatBox(); // 这个函数现在用来初始化 StackedWidget
    
    void addChatUserList(); 

private:
    QWidget *_side_bar;
    QWidget *_chat_list_wid;
    // QWidget *_chat_box_wid; // 这个不再需要，用 StackedWidget 代替

    CustomizeEdit *_search_edit;
    
    // [修改] 使用自定义的列表类
    ChatUserList *_chat_list;
    
    // [新增] 右侧页面管理
    QStackedWidget *_stacked_widget;
    ChatPage *_chat_page;

    // [新增] 加载状态标志
    bool _b_loading;
};