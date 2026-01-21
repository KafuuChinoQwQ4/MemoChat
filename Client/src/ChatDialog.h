#pragma once
#include <QDialog>
#include "global.h"

class QListWidget;
class QLineEdit;
class QLabel;
class QTextEdit;
class QStackedWidget;

class ChatDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();

private:
    void initUI(); // 初始化布局
    void addSideBar(); // 添加左侧边栏
    void addChatList(); // 添加中间列表区
    void addChatBox(); // 添加右侧聊天区

private:
    // UI 控件
    QWidget *_side_bar;      // 左侧边栏
    QWidget *_chat_list_wid; // 中间列表容器
    QWidget *_chat_box_wid;  // 右侧聊天容器

    // 搜索
    QLineEdit *_search_edit;
    
    // 列表
    QListWidget *_chat_list;
    
    // 聊天显示
    QLabel *_chat_title;
    QListWidget *_msg_show_list;
    QTextEdit *_chat_edit;
};