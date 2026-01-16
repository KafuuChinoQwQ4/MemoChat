#pragma once
#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class RegisterDialog : public QDialog {
    Q_OBJECT
public:
    explicit RegisterDialog(QWidget *parent = nullptr);

signals:
    void switchLogin(); // 这里的信号是“返回登录”

private:
    QPushButton *m_backBtn;
};