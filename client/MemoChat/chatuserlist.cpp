#include "chatuserlist.h"
#include<QScrollBar>
#include "usermgr.h"
#include <QTimer>
#include <QCoreApplication>

ChatUserList::ChatUserList(QWidget *parent):QListWidget(parent), _load_pending(false)
{
    Q_UNUSED(parent);
     this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
     this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    this->viewport()->installEventFilter(this);
}

bool ChatUserList::eventFilter(QObject *watched, QEvent *event)
{

    if (watched == this->viewport()) {
        if (event->type() == QEvent::Enter) {

            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {

            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }


    if (watched == this->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15;


        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);


        QScrollBar *scrollBar = this->verticalScrollBar();
        int maxScrollValue = scrollBar->maximum();
        int currentValue = scrollBar->value();


        if (maxScrollValue - currentValue <= 0) {
             auto b_loaded = UserMgr::GetInstance()->IsLoadChatFin();
             if(b_loaded){
                 return true;
             }

             if(_load_pending){
                 return true;
             }

            qDebug()<<"load more chat user";
            _load_pending = true;

            QTimer::singleShot(100, [this](){
                _load_pending = false;
                QCoreApplication::quit();
                });

            emit sig_loading_chat_user();
         }

        return true;
    }

    return QListWidget::eventFilter(watched, event);
}
