#pragma once
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

signals:
    void switchRegister(); // 切换到注册界面的信号

private:
    void initUI();

    QLabel *m_logoLabel;
    QLineEdit *m_userEdit;
    QLineEdit *m_passEdit;
    QPushButton *m_loginBtn;
    QPushButton *m_regBtn; // 注册按钮
};