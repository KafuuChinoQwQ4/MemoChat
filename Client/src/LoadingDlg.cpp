#include "LoadingDlg.h"

LoadingDlg::LoadingDlg(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground); // 设置背景透明
    setFixedSize(parent->size()); // 覆盖父窗口大小

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    
    _loading_lb = new QLabel(this);
    
    // 如果你有加载动画 gif，可以取消注释下面代码
    // QMovie *movie = new QMovie(":/res/loading.gif");
    // _loading_lb->setMovie(movie);
    // movie->start();
    
    _loading_lb->setText("Loading..."); // 暂时用文字代替
    _loading_lb->setStyleSheet("QLabel { color: white; font-size: 20px; font-weight: bold; background-color: rgba(0,0,0,150); padding: 20px; border-radius: 10px; }");
    _loading_lb->setAlignment(Qt::AlignCenter);

    layout->addWidget(_loading_lb);
}

LoadingDlg::~LoadingDlg()
{
}