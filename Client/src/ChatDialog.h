#pragma once
#include <QDialog>
#include "global.h"
#include "CustomizeEdit.h" // [新增]

class QListWidget;
class QLabel;
class QTextEdit;
class QStackedWidget;

class ChatDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();

private:
    void initUI(); 
    void addSideBar(); 
    void addChatList(); 
    void addChatBox(); 
    
    // [新增] 加载模拟的聊天用户数据
    void addChatUserList(); 

private:
    QWidget *_side_bar;
    QWidget *_chat_list_wid;
    QWidget *_chat_box_wid;

    // [修改] 使用自定义 Edit
    CustomizeEdit *_search_edit;
    
    QListWidget *_chat_list;
    QLabel *_chat_title;
    QListWidget *_msg_show_list;
    QTextEdit *_chat_edit;
};