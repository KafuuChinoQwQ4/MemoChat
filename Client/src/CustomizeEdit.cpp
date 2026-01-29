#include "CustomizeEdit.h"

CustomizeEdit::CustomizeEdit(QWidget *parent) : QLineEdit(parent), _max_len(0)
{
    connect(this, &QLineEdit::textChanged, this, &CustomizeEdit::limitTextLength);
}

void CustomizeEdit::SetMaxLength(int maxLen)
{
    _max_len = maxLen;
}

void CustomizeEdit::focusOutEvent(QFocusEvent *event)
{
    // 执行失去焦点时的处理逻辑
    QLineEdit::focusOutEvent(event);
    // 发送失去焦点的信号
    emit sig_foucus_out();
}

void CustomizeEdit::limitTextLength(QString text)
{
    if(_max_len <= 0){
        return;
    }

    QByteArray byteArray = text.toUtf8();
    if (byteArray.size() > _max_len) {
        byteArray = byteArray.left(_max_len);
        this->setText(QString::fromUtf8(byteArray));
    }
}