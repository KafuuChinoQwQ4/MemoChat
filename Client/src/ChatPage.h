#pragma once
#include <QWidget>
#include "global.h"

class QLabel;
class QTextEdit;
class ClickedBtn;
class ClickedLabel;
class ChatView; // [新增] 前向声明

class ChatPage : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPage(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *event) override;

private slots:
    void on_send_btn_clicked(); // [新增]

private:
    void initUI();

private:
    QLabel *_chat_title;
    ChatView *_chat_view; // [修改] 替换 QListWidget
    QTextEdit *_chat_edit;
    
    ClickedLabel *_emo_lb;
    ClickedLabel *_file_lb;
    
    ClickedBtn *_send_btn;
    ClickedBtn *_receive_btn;
};