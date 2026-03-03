#ifndef CUSTOMIZETEXTEDIT_H
#define CUSTOMIZETEXTEDIT_H
#include <QTextEdit>

class CustomizeTextEdit:public QTextEdit
{
    Q_OBJECT
public:
    CustomizeTextEdit(QWidget *parent = nullptr);

protected:
    void focusOutEvent(QFocusEvent *event) override
    {

        //qDebug() << "CustomizeEdit focusout";

        QTextEdit::focusOutEvent(event);

        emit sig_foucus_out();
    }

signals:
    void sig_foucus_out();
};

#endif // CUSTOMIZETEXTEDIT_H
