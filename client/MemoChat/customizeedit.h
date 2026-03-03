#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H
#include <QLineEdit>
#include <QDebug>

class CustomizeEdit: public QLineEdit
{
    Q_OBJECT
public:
    CustomizeEdit(QWidget *parent = nullptr);
    void SetMaxLength(int maxLen);
protected:
    void focusOutEvent(QFocusEvent *event) override
    {

        //qDebug() << "CustomizeEdit focusout";

        QLineEdit::focusOutEvent(event);

        emit sig_foucus_out();
    }
private:
    void limitTextLength(QString text) {
        if(_max_len <= 0){
            return;
        }

        QByteArray byteArray = text.toUtf8();

        if (byteArray.size() > _max_len) {
            byteArray = byteArray.left(_max_len);
            this->setText(QString::fromUtf8(byteArray));
        }
    }

    int _max_len;
signals:
    void sig_foucus_out();
};

#endif // CUSTOMIZEEDIT_H
