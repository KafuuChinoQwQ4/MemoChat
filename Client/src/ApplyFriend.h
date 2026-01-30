#pragma once
#include <QDialog>
#include <QMap>
#include <vector>
#include <memory>
#include "global.h"

class CustomizeEdit;
class ClickedLabel;
class FriendLabel;
class ClickedOnceLabel;
class QScrollArea;

class ApplyFriend : public QDialog
{
    Q_OBJECT

public:
    explicit ApplyFriend(QWidget *parent = nullptr);
    ~ApplyFriend();
    void InitTipLbs();
    void AddTipLbs(ClickedLabel*, QPoint cur_point, QPoint &next_point, int text_width, int text_height);
    bool eventFilter(QObject *obj, QEvent *event) override;
    void SetSearchInfo(std::shared_ptr<SearchInfo> si);

private:
    void initUI(); // 手动构建 UI
    void resetLabels();
    void addLabel(QString name);

    // 成员变量替代 ui->
    CustomizeEdit *_lb_ed;
    QWidget *_input_tip_wid;
    ClickedOnceLabel *_tip_lb;
    ClickedOnceLabel *_more_lb;
    QWidget *_more_lb_wid; // 包含“更多”标签的容器
    QWidget *_lb_list; // 标签展示区域
    QScrollArea *_scrollArea;
    QWidget *_scrollcontent;
    QWidget *_gridWidget; // 顶部已选标签容器
    CustomizeEdit *_name_ed;
    CustomizeEdit *_back_ed;
    QPushButton *_sure_btn;
    QPushButton *_cancel_btn;

    // 数据
    QMap<QString, ClickedLabel*> _add_labels;
    std::vector<QString> _add_label_keys;
    QPoint _label_point;
    
    QMap<QString, FriendLabel*> _friend_labels;
    std::vector<QString> _friend_label_keys;
    
    std::vector<QString> _tip_data;
    QPoint _tip_cur_point;
    std::shared_ptr<SearchInfo> _si;
    
    const int tip_offset = 5; // 间距常量
    const int MIN_APPLY_LABEL_ED_LEN = 50; // 输入框最小宽度
    QString add_prefix = "添加标签: ";

public slots:
    void ShowMoreLabel();
    void SlotLabelEnter();
    void SlotRemoveFriendLabel(QString);
    void SlotChangeFriendLabelByTip(QString, ClickLbState);
    void SlotLabelTextChange(const QString& text);
    void SlotLabelEditFinished();
    void SlotAddFirendLabelByClickTip(QString text);
    void SlotApplySure();
    void SlotApplyCancel();
};