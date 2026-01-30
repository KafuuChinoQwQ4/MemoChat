#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QEvent>
#include <QPainter>
#include <QStyleOption>
#include <QScrollBar>
#include <QTimer>

class ChatView : public QWidget
{
    Q_OBJECT
public:
    ChatView(QWidget *parent = Q_NULLPTR);
    void appendChatItem(QWidget *item);                 // 尾插
    void prependChatItem(QWidget *item);                // 头插
    void insertChatItem(QWidget *before, QWidget *item);// 中间插

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onVScrollBarMoved(int min, int max);

private:
    void initStyleSheet();

private:
    // QVBoxLayout *m_pVl; // 如果后续需要直接访问内部布局，可以取消注释并赋值
    QScrollArea *m_pScrollArea;
    bool isAppended;
};