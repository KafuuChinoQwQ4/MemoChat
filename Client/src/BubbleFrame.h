#pragma once
#include <QFrame>
#include <QHBoxLayout>
#include "global.h"

class QTextEdit;
class QLabel;

// === 基类：气泡边框 ===
class BubbleFrame : public QFrame
{
    Q_OBJECT
public:
    BubbleFrame(ChatRole role, QWidget *parent = nullptr);
    void setWidget(QWidget *w);
    
protected:
    void paintEvent(QPaintEvent *e) override;

private:
    QHBoxLayout *m_pHLayout;
    ChatRole m_role;
    int m_margin;
};

// === 文本气泡 ===
class TextBubble : public BubbleFrame
{
    Q_OBJECT
public:
    TextBubble(ChatRole role, const QString &text, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void setPlainText(const QString &text);
    void initStyleSheet();
    void adjustTextHeight();

private:
    QTextEdit *m_pTextEdit;
};

// === 图片气泡 ===
class PictureBubble : public BubbleFrame
{
    Q_OBJECT
public:
    PictureBubble(const QPixmap &picture, ChatRole role, QWidget *parent = nullptr);
};