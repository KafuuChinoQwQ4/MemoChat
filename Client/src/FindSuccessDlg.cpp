#include "FindSuccessDlg.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QCoreApplication>
#include <QDir>
#include <QPixmap>
#include <QDebug>
#include "ClickedBtn.h"
#include "ApplyFriend.h" // 引入 ApplyFriend 头文件

FindSuccessDlg::FindSuccessDlg(QWidget *parent) : QDialog(parent), _parent(parent)
{
    // 设置对话框属性
    setWindowTitle("添加");
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setModal(true);

    initUI();

    // 加载头像逻辑
    QString app_path = QCoreApplication::applicationDirPath();
    QString pix_path = QDir::toNativeSeparators(app_path +
                             QDir::separator() + "static"+QDir::separator()+"head_1.jpg");
    
    QPixmap head_pix(pix_path);
    if(head_pix.isNull()){
        head_pix.load(":/res/head_1.jpg");
        if(head_pix.isNull()) head_pix.load(":/res/KafuuChino.png");
    }

    if(!head_pix.isNull()){
        head_pix = head_pix.scaled(60, 60, // 这里的 size 需要和 initUI 中的 fixedSize 一致
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
        _head_lb->setPixmap(head_pix);
    }

    _add_friend_btn->SetState("normal","hover","press");
}

FindSuccessDlg::~FindSuccessDlg()
{
    qDebug()<<"FindSuccessDlg destruct";
}

void FindSuccessDlg::initUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    QWidget *bgWidget = new QWidget(this);
    bgWidget->setObjectName("find_success_bg");
    bgWidget->setFixedSize(280, 200); // 设定一个固定大小，避免太小
    
    QVBoxLayout *bgLayout = new QVBoxLayout(bgWidget);
    bgLayout->setContentsMargins(20, 20, 20, 20);

    // 头像
    _head_lb = new QLabel(bgWidget);
    _head_lb->setFixedSize(60, 60);
    _head_lb->setAlignment(Qt::AlignCenter);
    
    // 名字
    _name_lb = new QLabel(bgWidget);
    _name_lb->setAlignment(Qt::AlignCenter);
    _name_lb->setObjectName("find_user_name");
    QFont font;
    font.setPointSize(14);
    _name_lb->setFont(font);
    
    // 添加按钮
    _add_friend_btn = new ClickedBtn(bgWidget);
    _add_friend_btn->setFixedSize(100, 30);
    _add_friend_btn->setText("添加好友");
    _add_friend_btn->setObjectName("find_add_btn");

    bgLayout->addStretch();
    bgLayout->addWidget(_head_lb, 0, Qt::AlignHCenter);
    bgLayout->addSpacing(10);
    bgLayout->addWidget(_name_lb, 0, Qt::AlignHCenter);
    bgLayout->addSpacing(20);
    bgLayout->addWidget(_add_friend_btn, 0, Qt::AlignHCenter);
    bgLayout->addStretch();

    layout->addWidget(bgWidget);

    connect(_add_friend_btn, &ClickedBtn::clicked, this, &FindSuccessDlg::on_add_friend_btn_clicked);
}

void FindSuccessDlg::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    _name_lb->setText(si->_name);
    _si = si;
}

void FindSuccessDlg::on_add_friend_btn_clicked()
{
    this->hide();
    
    // 弹出加好友界面
    // 注意：ApplyFriend 需要在堆上创建，并设置 deleteOnClose 属性，或者由父对象管理生命周期
    // 这里为了简单，我们让它作为模态对话框运行，用完即删（或者交给 Qt 对象树）
    auto applyFriend = new ApplyFriend(_parent);
    applyFriend->SetSearchInfo(_si);
    applyFriend->setModal(true);
    applyFriend->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动释放内存
    applyFriend->show();
}