#pragma once
#include <QDialog>
#include "global.h"

class ClickedBtn;
class QLabel;

class FindFailDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FindFailDlg(QWidget *parent = nullptr);
    ~FindFailDlg();

private slots:
    void on_fail_sure_btn_clicked();

private:
    void initUI();
    QLabel * _fail_tip_lb;
    ClickedBtn * _fail_sure_btn;
};