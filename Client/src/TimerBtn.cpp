#include "TimerBtn.h"
#include <QMouseEvent>
#include <QDebug>

TimerBtn::TimerBtn(QWidget *parent) : QPushButton(parent), _counter(10)
{
    _timer = new QTimer(this);

    connect(_timer, &QTimer::timeout, [this](){
        _counter--;
        if(_counter <= 0){
            _timer->stop();
            _counter = 10;
            this->setText("获取");
            this->setEnabled(true);
            return;
        }
        this->setText(QString::number(_counter));
    });
}

TimerBtn::~TimerBtn()
{
    _timer->stop();
}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        // 只有当按钮启用时才处理（虽然setEnabled(false)通常会屏蔽事件，但在某些样式下保险起见）
        if(this->isEnabled()){
            qDebug() << "TimerBtn clicked";
            this->setEnabled(false);
            this->setText(QString::number(_counter));
            _timer->start(1000); // 1秒触发一次
            emit clicked(); // 手动发射 clicked 信号
        }
    }
    // 阻止基类处理（或者根据需求调用）
    // QPushButton::mouseReleaseEvent(e); 
}