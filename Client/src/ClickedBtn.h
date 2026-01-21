#pragma once
#include <QPushButton>
#include <QEvent>

class ClickedBtn : public QPushButton
{
    Q_OBJECT
public:
    ClickedBtn(QWidget *parent = nullptr);
    ~ClickedBtn();
    
    // 设置状态对应的属性值
    void SetState(QString normal, QString hover, QString press);

protected:
    void enterEvent(QEnterEvent *event) override; // Qt6 use QEnterEvent
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString _normal;
    QString _hover;
    QString _press;
};