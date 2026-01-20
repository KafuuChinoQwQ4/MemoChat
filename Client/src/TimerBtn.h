#pragma once
#include <QPushButton>
#include <QTimer>

class TimerBtn : public QPushButton
{
    Q_OBJECT
public:
    TimerBtn(QWidget *parent = nullptr);
    ~TimerBtn();

    // 重写鼠标释放事件，处理点击逻辑
    virtual void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QTimer *_timer;
    int _counter;
};