#pragma once
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "ClickedLabel.h" // <--- 必须包含这个

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

signals:
    void switchRegister();
    void switchReset(); // <--- 切换到重置密码信号

private:
    void initUI();

    QLabel *m_logoLabel;
    QLineEdit *m_userEdit;
    QLineEdit *m_passEdit;
    QPushButton *m_loginBtn;
    QPushButton *m_regBtn;
    
    // 忘记密码标签
    ClickedLabel *m_forgetLabel; 
};