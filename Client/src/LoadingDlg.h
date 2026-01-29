#pragma once
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QMovie>

class LoadingDlg : public QDialog
{
    Q_OBJECT
public:
    explicit LoadingDlg(QWidget *parent = nullptr);
    ~LoadingDlg();

private:
    QLabel *_loading_lb;
};