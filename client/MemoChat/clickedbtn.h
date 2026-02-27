#ifndef CLICKEDBTN_H
#define CLICKEDBTN_H

#include <QPushButton>
#include <QEnterEvent>

class ClickedBtn : public QPushButton
{
    Q_OBJECT
public:
    explicit ClickedBtn(QWidget *parent = nullptr);
    ~ClickedBtn();

    void SetState(QString normal, QString hover, QString press);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString _normal;
    QString _hover;
    QString _press;
};

#endif // CLICKEDBTN_H
