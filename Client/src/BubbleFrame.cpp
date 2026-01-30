#include "BubbleFrame.h"
#include <QPainter>
#include <QTextEdit>
#include <QLabel>
#include <QDebug>
#include <QTextBlock>
#include <QTextDocument>

const int WIDTH_SANJIAO = 8; // 小三角的宽度

// ================= BubbleFrame 实现 =================
BubbleFrame::BubbleFrame(ChatRole role, QWidget *parent)
    : QFrame(parent)
    , m_role(role)
    , m_margin(3)
{
    m_pHLayout = new QHBoxLayout();
    if(m_role == ChatRole::Self)
        // 自己发的消息，三角在右边，右边距要大一点
        m_pHLayout->setContentsMargins(m_margin, m_margin, WIDTH_SANJIAO + m_margin, m_margin);
    else
        // 别人发的消息，三角在左边，左边距要大一点
        m_pHLayout->setContentsMargins(WIDTH_SANJIAO + m_margin, m_margin, m_margin, m_margin);

    this->setLayout(m_pHLayout);
}

void BubbleFrame::setWidget(QWidget *w)
{
    if(m_pHLayout->count() > 0) return;
    m_pHLayout->addWidget(w);
}

void BubbleFrame::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿

    if(m_role == ChatRole::Other)
    {
        // === 别人发的消息 (白色背景) ===
        QColor bk_color(Qt::white);
        painter.setBrush(QBrush(bk_color));
        
        // 1. 画圆角矩形 (向右偏移 WIDTH_SANJIAO)
        QRect bk_rect = QRect(WIDTH_SANJIAO, 0, this->width() - WIDTH_SANJIAO, this->height());
        painter.drawRoundedRect(bk_rect, 5, 5);
        
        // 2. 画小三角 (左侧)
        QPointF points[3] = {
            QPointF(bk_rect.x(), 12),
            QPointF(bk_rect.x(), 12 + WIDTH_SANJIAO + 2),
            QPointF(bk_rect.x() - WIDTH_SANJIAO, 10 + WIDTH_SANJIAO - WIDTH_SANJIAO / 2),
        };
        painter.drawPolygon(points, 3);
    }
    else
    {
        // === 自己发的消息 (绿色背景) ===
        QColor bk_color(158, 234, 106);
        painter.setBrush(QBrush(bk_color));
        
        // 1. 画圆角矩形
        QRect bk_rect = QRect(0, 0, this->width() - WIDTH_SANJIAO, this->height());
        painter.drawRoundedRect(bk_rect, 5, 5);
        
        // 2. 画小三角 (右侧)
        QPointF points[3] = {
            QPointF(bk_rect.x() + bk_rect.width(), 12),
            QPointF(bk_rect.x() + bk_rect.width(), 12 + WIDTH_SANJIAO + 2),
            QPointF(bk_rect.x() + bk_rect.width() + WIDTH_SANJIAO, 10 + WIDTH_SANJIAO - WIDTH_SANJIAO / 2),
        };
        painter.drawPolygon(points, 3);
    }

    QFrame::paintEvent(e);
}

// ================= TextBubble 实现 =================
TextBubble::TextBubble(ChatRole role, const QString &text, QWidget *parent)
    : BubbleFrame(role, parent)
{
    m_pTextEdit = new QTextEdit();
    m_pTextEdit->setReadOnly(true);
    m_pTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->installEventFilter(this);
    
    QFont font("Microsoft YaHei");
    font.setPointSize(12);
    m_pTextEdit->setFont(font);
    
    setPlainText(text);
    setWidget(m_pTextEdit);
    initStyleSheet();
}

void TextBubble::initStyleSheet()
{
    m_pTextEdit->setStyleSheet("QTextEdit{background:transparent;border:none;}");
}

void TextBubble::setPlainText(const QString &text)
{
    m_pTextEdit->setPlainText(text);
    
    // 计算文本宽度
    qreal doc_margin = m_pTextEdit->document()->documentMargin();
    int margin_left = this->layout()->contentsMargins().left();
    int margin_right = this->layout()->contentsMargins().right();
    
    QFontMetricsF fm(m_pTextEdit->font());
    QTextDocument *doc = m_pTextEdit->document();
    int max_width = 0;
    
    // 遍历每一行找到最宽的那一行
    for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())
    {
        // Qt6 使用 horizontalAdvance 替代 width
        int txtW = int(fm.horizontalAdvance(it.text()));
        max_width = max_width < txtW ? txtW : max_width;
    }
    
    // 设置最大宽度 (加一些余量防止换行)
    setMaximumWidth(max_width + doc_margin * 2 + (margin_left + margin_right));
}

bool TextBubble::eventFilter(QObject *o, QEvent *e)
{
    if(m_pTextEdit == o && e->type() == QEvent::Paint)
    {
        adjustTextHeight();
    }
    return BubbleFrame::eventFilter(o, e);
}

void TextBubble::adjustTextHeight()
{
    qreal doc_margin = m_pTextEdit->document()->documentMargin();
    QTextDocument *doc = m_pTextEdit->document();
    qreal text_height = 0;
    
    // 累加每一行的高度
    for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())
    {
        QTextLayout *pLayout = it.layout();
        QRectF text_rect = pLayout->boundingRect();
        text_height += text_rect.height();
    }
    
    int vMargin = this->layout()->contentsMargins().top();
    setFixedHeight(text_height + doc_margin * 2 + vMargin * 2);
}

// ================= PictureBubble 实现 =================
#define PIC_MAX_WIDTH 160
#define PIC_MAX_HEIGHT 90

PictureBubble::PictureBubble(const QPixmap &picture, ChatRole role, QWidget *parent)
    : BubbleFrame(role, parent)
{
    QLabel *lb = new QLabel();
    lb->setScaledContents(true);
    QPixmap pix = picture.scaled(QSize(PIC_MAX_WIDTH, PIC_MAX_HEIGHT), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    lb->setPixmap(pix);
    this->setWidget(lb);

    int left_margin = this->layout()->contentsMargins().left();
    int right_margin = this->layout()->contentsMargins().right();
    int v_margin = this->layout()->contentsMargins().bottom();
    
    setFixedSize(pix.width() + left_margin + right_margin, pix.height() + v_margin * 2);
}