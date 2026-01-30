#include "ChatView.h"
#include <QDebug>

ChatView::ChatView(QWidget *parent)
    : QWidget(parent)
    , isAppended(false)
{
    // 1. 最外层布局 (MainLayout)
    QVBoxLayout *pMainLayout = new QVBoxLayout();
    this->setLayout(pMainLayout);
    pMainLayout->setContentsMargins(0, 0, 0, 0); // Qt6 写法: setMargin 已废弃

    // 2. 滚动区域 (ScrollArea)
    m_pScrollArea = new QScrollArea();
    m_pScrollArea->setObjectName("chat_area");
    pMainLayout->addWidget(m_pScrollArea);

    // 3. 滚动区域的内容部件 (Widget)
    QWidget *w = new QWidget(this);
    w->setObjectName("chat_bg");
    w->setAutoFillBackground(true);

    // 4. 内容部件的垂直布局 (VLayout)
    QVBoxLayout *pVLayout_1 = new QVBoxLayout();
    pVLayout_1->addWidget(new QWidget(), 100000); // 这是一个弹簧 Widget，用于将聊天记录顶上去
    w->setLayout(pVLayout_1);
    
    m_pScrollArea->setWidget(w); // 设置 ScrollArea 的内容

    // 5. 滚动条处理
    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 默认隐藏，通过事件过滤器控制
    QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
    connect(pVScrollBar, &QScrollBar::rangeChanged, this, &ChatView::onVScrollBarMoved);

    // 6. 将垂直 ScrollBar 悬浮在 ScrollArea 上
    // 原理：给 ScrollArea 设置一个布局，把 ScrollBar 放进去，并右对齐
    QHBoxLayout *pHLayout_2 = new QHBoxLayout();
    pHLayout_2->addWidget(pVScrollBar, 0, Qt::AlignRight);
    pHLayout_2->setContentsMargins(0, 0, 0, 0); // Qt6 写法
    m_pScrollArea->setLayout(pHLayout_2);
    pVScrollBar->setHidden(true); // 初始隐藏

    m_pScrollArea->setWidgetResizable(true); // 让内容 Widget 自适应 ScrollArea 大小
    m_pScrollArea->installEventFilter(this); // 安装事件过滤器检测鼠标进入/离开
    
    initStyleSheet();
}

void ChatView::appendChatItem(QWidget *item)
{
    // 获取内容 Widget 的布局
    QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    // 在弹簧 Widget (count-1) 之前插入新消息
    vl->insertWidget(vl->count()-1, item);
    isAppended = true;
}

void ChatView::prependChatItem(QWidget *item)
{
    // 头插预留接口
}

void ChatView::insertChatItem(QWidget *before, QWidget *item)
{
    // 中间插预留接口
}

bool ChatView::eventFilter(QObject *o, QEvent *e)
{
    // 鼠标进入 ScrollArea 时显示滚动条
    if(e->type() == QEvent::Enter && o == m_pScrollArea)
    {
        // 只有当内容超出可视区域（maximum > 0）时才显示
        m_pScrollArea->verticalScrollBar()->setHidden(m_pScrollArea->verticalScrollBar()->maximum() == 0);
    }
    // 鼠标离开 ScrollArea 时隐藏滚动条
    else if(e->type() == QEvent::Leave && o == m_pScrollArea)
    {
         m_pScrollArea->verticalScrollBar()->setHidden(true);
    }
    return QWidget::eventFilter(o, e);
}

void ChatView::paintEvent(QPaintEvent *event)
{
    // 支持 QSS 样式表
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatView::onVScrollBarMoved(int min, int max)
{
    // 如果是新添加了消息，自动滚动到底部
    if(isAppended) 
    {
        QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
        pVScrollBar->setSliderPosition(pVScrollBar->maximum());
        
        // 500毫秒延时重置标志位，防止短时间内多次触发导致的问题
        QTimer::singleShot(500, [this]()
        {
            isAppended = false;
        });
    }
}

void ChatView::initStyleSheet()
{
    // 可以在这里设置特定的样式，或者依赖外部 QSS
    // 例如：this->setStyleSheet("background-color: white;");
}