#pragma once
#include <QWidget>
#include "global.h"

class QLabel;
class QListWidget;
class QTextEdit;
class ClickedBtn;
class ClickedLabel;

class ChatPage : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPage(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *event) override;

private:
    void initUI();

private:
    QLabel *_chat_title;
    QListWidget *_msg_show_list;
    QTextEdit *_chat_edit;
    
    // 工具栏按钮
    ClickedLabel *_emo_lb;
    ClickedLabel *_file_lb;
    
    // 发送接收按钮
    ClickedBtn *_send_btn;
    ClickedBtn *_receive_btn; // 示例用，虽然实际只有发送
};