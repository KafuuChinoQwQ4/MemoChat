#include "RegisterDialog.h"

RegisterDialog::RegisterDialog(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel("这里是注册界面", this);
    label->setAlignment(Qt::AlignCenter);
    
    m_backBtn = new QPushButton("返回登录", this);
    
    layout->addWidget(label);
    layout->addWidget(m_backBtn);
    
    connect(m_backBtn, &QPushButton::clicked, this, &RegisterDialog::switchLogin);
}