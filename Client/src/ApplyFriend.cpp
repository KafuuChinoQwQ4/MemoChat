#include "ApplyFriend.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QDebug>
#include "CustomizeEdit.h"
#include "ClickedLabel.h"
#include "FriendLabel.h"
#include "ClickedOnceLabel.h"
#include "UserMgr.h" // SetSearchInfo 中用到

ApplyFriend::ApplyFriend(QWidget *parent) :
    QDialog(parent), _label_point(2,6)
{
    // 隐藏对话框标题栏
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("ApplyFriend");
    this->setModal(true);

    initUI(); // 构建界面

    _name_ed->setPlaceholderText(tr("恋恋风辰"));
    _lb_ed->setPlaceholderText("搜索、添加标签");
    _back_ed->setPlaceholderText("燃烧的胸毛");

    _lb_ed->SetMaxLength(21);
    // _lb_ed->move(2, 2); // 已经在 addLabel/resetLabel 逻辑中处理，这里不需要初始 move，或者在 initUI 处理
    _lb_ed->setFixedHeight(20);
    _lb_ed->setMaxLength(10);
    _input_tip_wid->hide();

    _tip_cur_point = QPoint(5, 5);

    _tip_data = { "同学","家人","菜鸟教程","C++ Primer","Rust 程序设计",
                             "父与子学Python","nodejs开发指南","go 语言开发指南",
                                "游戏伙伴","金融投资","微信读书","拼多多拼友" };

    connect(_more_lb, &ClickedOnceLabel::clicked, this, &ApplyFriend::ShowMoreLabel);
    InitTipLbs();
    
    //链接输入标签回车事件
    connect(_lb_ed, &CustomizeEdit::returnPressed, this, &ApplyFriend::SlotLabelEnter);
    connect(_lb_ed, &CustomizeEdit::textChanged, this, &ApplyFriend::SlotLabelTextChange);
    connect(_lb_ed, &CustomizeEdit::editingFinished, this, &ApplyFriend::SlotLabelEditFinished);
    connect(_tip_lb, &ClickedOnceLabel::clicked, this, &ApplyFriend::SlotAddFirendLabelByClickTip);

    _scrollArea->horizontalScrollBar()->setHidden(true);
    _scrollArea->verticalScrollBar()->setHidden(true);
    _scrollArea->installEventFilter(this);
    
    // 自定义按钮通常是 ClickedBtn，但文档里用了 QPushButton 配合 QSS，这里假设是 QPushButton 或者是 UserDefined Btn
    // 如果是普通 QPushButton，SetState 方法是不存在的。
    // 根据文档 QSS，ui->sure_btn 和 cancel_btn 使用了 state 属性。
    // 我们这里假设你用了 ClickedBtn (如果有的话) 或者手动 setProperty。
    // 为了兼容文档调用 SetState，我们需要确认 sure_btn 是什么类型。
    // 之前的文件里有 ClickedBtn，如果是那个的话就好办。
    // 暂时用 QPushButton + setProperty 模拟，或者你需要引入 ClickedBtn。
    // 这里为了逻辑跑通，我使用 ClickedBtn。你需要 #include "ClickedBtn.h"
    // 如果没有 ClickedBtn，请删除 SetState 调用，直接用 QSS hover 伪状态。
    // 文档里有 clickedbtn.cpp，所以这里用 ClickedBtn。
    
    // _sure_btn->SetState... (这里先注释掉，因为声明里写的是 QPushButton)
    // 建议：在 .h 里把 _sure_btn 改成 ClickedBtn* 类型
    
    connect(_cancel_btn, &QPushButton::clicked, this, &ApplyFriend::SlotApplyCancel);
    connect(_sure_btn, &QPushButton::clicked, this, &ApplyFriend::SlotApplySure);
}

ApplyFriend::~ApplyFriend()
{
    qDebug()<< "ApplyFriend destruct";
}

void ApplyFriend::initUI() {
    this->setFixedSize(600, 450); // 设定一个初始大小
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    
    // 顶部区域 Widget
    QWidget *apply_wid = new QWidget(this);
    apply_wid->setObjectName("apply_wid");
    QVBoxLayout *applyLayout = new QVBoxLayout(apply_wid);
    applyLayout->setContentsMargins(20,20,20,20);
    applyLayout->setSpacing(10);

    // Row 1: 申请人姓名
    QWidget *row1 = new QWidget(apply_wid);
    QHBoxLayout *row1Layout = new QHBoxLayout(row1);
    row1Layout->setContentsMargins(0,0,0,0);
    QLabel *l1 = new QLabel("申请人:", row1);
    l1->setFixedWidth(60);
    _name_ed = new CustomizeEdit(row1);
    _name_ed->setObjectName("name_ed");
    _name_ed->setReadOnly(true); // 或者是可编辑，看需求
    row1Layout->addWidget(l1);
    row1Layout->addWidget(_name_ed);

    // Row 2: 备注
    QWidget *row2 = new QWidget(apply_wid);
    QHBoxLayout *row2Layout = new QHBoxLayout(row2);
    row2Layout->setContentsMargins(0,0,0,0);
    QLabel *l2 = new QLabel("备注:", row2);
    l2->setFixedWidth(60);
    _back_ed = new CustomizeEdit(row2);
    _back_ed->setObjectName("back_ed");
    row2Layout->addWidget(l2);
    row2Layout->addWidget(_back_ed);

    // Row 3: 标签输入区 (gridWidget)
    QWidget *row3 = new QWidget(apply_wid);
    QHBoxLayout *row3Layout = new QHBoxLayout(row3);
    row3Layout->setContentsMargins(0,0,0,0);
    QLabel *l3 = new QLabel("标签:", row3);
    l3->setFixedWidth(60);
    l3->setAlignment(Qt::AlignTop);

    _gridWidget = new QWidget(row3);
    _gridWidget->setObjectName("gridWidget");
    _gridWidget->setMinimumHeight(50); // 给一点初始高度
    
    // lb_ed 放在 gridWidget 里
    _lb_ed = new CustomizeEdit(_gridWidget);
    _lb_ed->setObjectName("lb_ed");
    _lb_ed->move(2, 2);

    row3Layout->addWidget(l3);
    row3Layout->addWidget(_gridWidget);

    // Row 4: 标签展示列表 (lb_list inside ScrollArea)
    QWidget *row4 = new QWidget(apply_wid);
    QVBoxLayout *row4Layout = new QVBoxLayout(row4);
    row4Layout->setContentsMargins(0,10,0,0);
    
    // 更多标签 头部
    QWidget *headerWid = new QWidget(row4);
    QHBoxLayout *hl = new QHBoxLayout(headerWid);
    hl->setContentsMargins(0,0,0,0);
    QLabel *tagTitle = new QLabel("添加标签", headerWid);
    _more_lb_wid = new QWidget(headerWid);
    QHBoxLayout *moreLayout = new QHBoxLayout(_more_lb_wid);
    moreLayout->setContentsMargins(0,0,0,0);
    _more_lb = new ClickedOnceLabel(_more_lb_wid);
    _more_lb->setObjectName("more_lb"); // 对应 QSS 的箭头图标
    _more_lb->setFixedSize(20,20);
    moreLayout->addStretch();
    moreLayout->addWidget(_more_lb);
    
    hl->addWidget(tagTitle);
    hl->addWidget(_more_lb_wid);

    _scrollArea = new QScrollArea(row4);
    _scrollArea->setObjectName("scrollArea");
    _scrollArea->setWidgetResizable(true);
    _scrollcontent = new QWidget();
    _scrollcontent->setObjectName("scrollcontent");
    _lb_list = new QWidget(_scrollcontent); // 标签放置在这个 widget 上
    _lb_list->setObjectName("lb_list");
    _lb_list->setGeometry(0, 0, 500, 1000); // 初始大小，后面会动态 resize
    _scrollArea->setWidget(_scrollcontent);
    
    // 自动补全提示框 (浮动在界面上，或者在布局里)
    // 根据文档逻辑，它是 ui->input_tip_wid
    _input_tip_wid = new QWidget(apply_wid); // 放在 apply_wid 上层
    _input_tip_wid->setObjectName("input_tip_wid");
    _input_tip_wid->hide();
    _input_tip_wid->setGeometry(70, 150, 200, 30); // 示例位置，需要根据 lb_ed 动态调整，或者固定位置
    // 在 input_tip_wid 里放一个 tip_lb
    QHBoxLayout *tipLayout = new QHBoxLayout(_input_tip_wid);
    tipLayout->setContentsMargins(5,0,0,0);
    _tip_lb = new ClickedOnceLabel(_input_tip_wid);
    _tip_lb->setObjectName("tip_lb");
    tipLayout->addWidget(_tip_lb);


    row4Layout->addWidget(headerWid);
    row4Layout->addWidget(_scrollArea);

    applyLayout->addWidget(row1);
    applyLayout->addWidget(row2);
    applyLayout->addWidget(row3);
    applyLayout->addWidget(row4);
    
    // 底部按钮区域
    QWidget *sure_wid = new QWidget(this);
    sure_wid->setObjectName("apply_sure_wid");
    QHBoxLayout *btnLayout = new QHBoxLayout(sure_wid);
    btnLayout->setContentsMargins(20, 10, 20, 10);
    btnLayout->addStretch();
    
    _sure_btn = new QPushButton("确认", sure_wid);
    _sure_btn->setObjectName("sure_btn");
    _sure_btn->setFixedSize(80, 30);
    
    _cancel_btn = new QPushButton("取消", sure_wid);
    _cancel_btn->setObjectName("cancel_btn");
    _cancel_btn->setFixedSize(80, 30);
    
    btnLayout->addWidget(_sure_btn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(_cancel_btn);

    mainLayout->addWidget(apply_wid);
    mainLayout->addWidget(sure_wid);
}

// === 下面是文档提供的逻辑代码，稍作变量名适配 ===

void ApplyFriend::InitTipLbs()
{
    int lines = 1;
    for(int i = 0; i < _tip_data.size(); i++){

        auto* lb = new ClickedLabel(_lb_list); // 适配变量名
        lb->SetState("normal", "hover", "pressed", "selected_normal",
            "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font()); 
        int textWidth = fontMetrics.horizontalAdvance(lb->text()); // Qt6 Use horizontalAdvance
        int textHeight = fontMetrics.height(); 

        if (_tip_cur_point.x() + textWidth + tip_offset > _lb_list->width()) {
            lines++;
            if (lines > 2) {
                delete lb;
                return;
            }

            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

        }

       auto next_point = _tip_cur_point;

       AddTipLbs(lb, _tip_cur_point,next_point, textWidth, textHeight);

       _tip_cur_point = next_point;
    }
}

void ApplyFriend::AddTipLbs(ClickedLabel* lb, QPoint cur_point, QPoint& next_point, int text_width, int text_height)
{
    lb->move(cur_point);
    lb->show();
    _add_labels.insert(lb->text(), lb);
    _add_label_keys.push_back(lb->text());
    next_point.setX(lb->pos().x() + text_width + 15);
    next_point.setY(lb->pos().y());
}

bool ApplyFriend::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == _scrollArea && event->type() == QEvent::Enter)
    {
        _scrollArea->verticalScrollBar()->setHidden(false);
    }
    else if (obj == _scrollArea && event->type() == QEvent::Leave)
    {
        _scrollArea->verticalScrollBar()->setHidden(true);
    }
    return QObject::eventFilter(obj, event);
}

void ApplyFriend::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    _si = si;
    auto applyname = UserMgr::GetInstance()->GetName();
    auto bakname = si->_name;
    _name_ed->setText(applyname);
    _back_ed->setText(bakname);
}

void ApplyFriend::ShowMoreLabel()
{
    qDebug()<< "receive more label clicked";
    _more_lb_wid->hide();

    _lb_list->setFixedWidth(325); // 这里可能需要根据实际 Layout 调整
    _tip_cur_point = QPoint(5, 5);
    auto next_point = _tip_cur_point;
    int textWidth;
    int textHeight;
    //重排现有的label
    for(auto & added_key : _add_label_keys){
        auto added_lb = _add_labels[added_key];

        QFontMetrics fontMetrics(added_lb->font()); 
        textWidth = fontMetrics.horizontalAdvance(added_lb->text()); 
        textHeight = fontMetrics.height(); 

        if(_tip_cur_point.x() +textWidth + tip_offset > _lb_list->width()){
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y()+textHeight+15);
        }
        added_lb->move(_tip_cur_point);

        next_point.setX(added_lb->pos().x() + textWidth + 15);
        next_point.setY(_tip_cur_point.y());

        _tip_cur_point = next_point;
    }

    //添加未添加的
    for(int i = 0; i < _tip_data.size(); i++){
        auto iter = _add_labels.find(_tip_data[i]);
        if(iter != _add_labels.end()){
            continue;
        }

        auto* lb = new ClickedLabel(_lb_list);
        lb->SetState("normal", "hover", "pressed", "selected_normal",
            "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font()); 
        textWidth = fontMetrics.horizontalAdvance(lb->text()); 
        textHeight = fontMetrics.height(); 

        if (_tip_cur_point.x() + textWidth + tip_offset > _lb_list->width()) {
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
        }

         next_point = _tip_cur_point;
        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
        _tip_cur_point = next_point;
    }

   int diff_height = next_point.y() + textHeight + tip_offset - _lb_list->height();
   _lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);
   // _scrollcontent->setFixedHeight(_scrollcontent->height()+diff_height); 
   // QScrollArea 自动处理 content 大小通常更好，如果 widgetResizable 为 true
   _scrollcontent->setMinimumHeight(_lb_list->height());
}

void ApplyFriend::resetLabels()
{
    auto max_width = _gridWidget->width();
    auto label_height = 0;
    for(auto iter = _friend_labels.begin(); iter != _friend_labels.end(); iter++){
        if( _label_point.x() + iter.value()->width() > max_width) {
            _label_point.setY(_label_point.y()+iter.value()->height()+6);
            _label_point.setX(2);
        }

        iter.value()->move(_label_point);
        iter.value()->show();

        _label_point.setX(_label_point.x()+iter.value()->width()+2);
        _label_point.setY(_label_point.y());
        label_height = iter.value()->height();
    }

    if(_friend_labels.isEmpty()){
         _lb_ed->move(_label_point);
         return;
    }

    if(_label_point.x() + MIN_APPLY_LABEL_ED_LEN > _gridWidget->width()){
        _lb_ed->move(2,_label_point.y()+label_height+6);
    }else{
         _lb_ed->move(_label_point);
    }
}

void ApplyFriend::addLabel(QString name)
{
    if (_friend_labels.find(name) != _friend_labels.end()) {
        return;
    }

    auto tmplabel = new FriendLabel(_gridWidget);
    tmplabel->SetText(name);
    tmplabel->setObjectName("FriendLabel");
    
    // 需要 show 之后才能获取准确的 width/height
    tmplabel->show();
    tmplabel->adjustSize();

    auto max_width = _gridWidget->width();
    
    if (_label_point.x() + tmplabel->width() > max_width) {
        _label_point.setY(_label_point.y() + tmplabel->height() + 6);
        _label_point.setX(2);
    }

    tmplabel->move(_label_point);
    _friend_labels[tmplabel->Text()] = tmplabel;
    _friend_label_keys.push_back(tmplabel->Text());

    connect(tmplabel, &FriendLabel::sig_close, this, &ApplyFriend::SlotRemoveFriendLabel);

    _label_point.setX(_label_point.x() + tmplabel->width() + 2);

    if (_label_point.x() + MIN_APPLY_LABEL_ED_LEN > _gridWidget->width()) {
        _lb_ed->move(2, _label_point.y() + tmplabel->height() + 2);
    }
    else {
        _lb_ed->move(_label_point);
    }

    _lb_ed->clear();

    if (_gridWidget->height() < _label_point.y() + tmplabel->height() + 2) {
        _gridWidget->setFixedHeight(_label_point.y() + tmplabel->height() * 2 + 2);
    }
}

void ApplyFriend::SlotLabelEnter()
{
    if(_lb_ed->text().isEmpty()){
        return;
    }

    auto text = _lb_ed->text();
    addLabel(_lb_ed->text());

    _input_tip_wid->hide();
    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text);
    }

    auto find_add = _add_labels.find(text);
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }

    auto* lb = new ClickedLabel(_lb_list);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);
    
    QFontMetrics fontMetrics(lb->font()); 
    int textWidth = fontMetrics.horizontalAdvance(lb->text()); 
    int textHeight = fontMetrics.height(); 

    if (_tip_cur_point.x() + textWidth + tip_offset + 3 > _lb_list->width()) {
        _tip_cur_point.setX(5);
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
    }

    auto next_point = _tip_cur_point;

    AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
    _tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - _lb_list->height();
    _lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);

    // _scrollcontent->setFixedHeight(_scrollcontent->height() + diff_height);
    _scrollcontent->setMinimumHeight(_lb_list->height());
}

void ApplyFriend::SlotRemoveFriendLabel(QString name)
{
    qDebug() << "receive close signal";

    _label_point.setX(2);
    _label_point.setY(6);

   auto find_iter = _friend_labels.find(name);
   if(find_iter == _friend_labels.end()){
       return;
   }

   auto find_key = _friend_label_keys.end();
   for(auto iter = _friend_label_keys.begin(); iter != _friend_label_keys.end(); iter++){
       if(*iter == name){
           find_key = iter;
           break;
       }
   }
   if(find_key != _friend_label_keys.end()){
      _friend_label_keys.erase(find_key);
   }

   delete find_iter.value();
   _friend_labels.erase(find_iter);

   resetLabels();

   auto find_add = _add_labels.find(name);
   if(find_add == _add_labels.end()){
        return;
   }
   find_add.value()->ResetNormalState();
}

void ApplyFriend::SlotChangeFriendLabelByTip(QString lbtext, ClickLbState state)
{
    auto find_iter = _add_labels.find(lbtext);
    if(find_iter == _add_labels.end()){
        return;
    }
    if(state == ClickLbState::Selected){
        addLabel(lbtext);
        return;
    }
    if(state == ClickLbState::Normal){
        SlotRemoveFriendLabel(lbtext);
        return;
    }
}

void ApplyFriend::SlotLabelTextChange(const QString& text)
{
    if (text.isEmpty()) {
        _tip_lb->setText("");
        _input_tip_wid->hide();
        return;
    }

    auto iter = std::find(_tip_data.begin(), _tip_data.end(), text);
    if (iter == _tip_data.end()) {
        auto new_text = add_prefix + text;
        _tip_lb->setText(new_text);
        _input_tip_wid->show();
        return;
    }
    _tip_lb->setText(text);
    _input_tip_wid->show();
}

void ApplyFriend::SlotLabelEditFinished()
{
    _input_tip_wid->hide();
}

void ApplyFriend::SlotAddFirendLabelByClickTip(QString text)
{
    int index = text.indexOf(add_prefix);
    if (index != -1) {
        text = text.mid(index + add_prefix.length());
    }
    addLabel(text);

    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text);
    }
   
    auto find_add = _add_labels.find(text);
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }
     
    auto* lb = new ClickedLabel(_lb_list);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);
   
    QFontMetrics fontMetrics(lb->font());
    int textWidth = fontMetrics.horizontalAdvance(lb->text()); 
    int textHeight = fontMetrics.height();

    if (_tip_cur_point.x() + textWidth+ tip_offset+3 > _lb_list->width()) {
        _tip_cur_point.setX(5);
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
    }

    auto next_point = _tip_cur_point;
    AddTipLbs(lb, _tip_cur_point, next_point, textWidth,textHeight);
    _tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - _lb_list->height();
    _lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);
    // _scrollcontent->setFixedHeight(_scrollcontent->height()+ diff_height );
    _scrollcontent->setMinimumHeight(_lb_list->height());
}

void ApplyFriend::SlotApplyCancel()
{
    qDebug() << "Slot Apply Cancel";
    this->hide();
    deleteLater();
}

void ApplyFriend::SlotApplySure()
{
    qDebug()<<"Slot Apply Sure called" ;
    // 实际逻辑中这里应该发送网络请求
    this->hide();
    deleteLater();
}