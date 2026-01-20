#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QJsonObject>
#include "global.h"
#include "TimerBtn.h"
#include "ClickedLabel.h"

class ResetDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ResetDialog(QWidget *parent = nullptr);
    ~ResetDialog();

private slots:
    void onGetCodeClicked();
    void onSureClicked();
    void onReturnClicked();
    void slot_reset_mod_finish(ReqId id, QString res, ErrorCodes err);

    // 校验槽函数
    void checkUserValid();
    void checkEmailValid();
    void checkPassValid();
    void checkVarifyValid();

signals:
    void switchLogin();

private:
    void initUI();
    void initHttpHandlers();
    void showTip(QString str, bool b_ok);
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);

    // UI 控件
    QLineEdit *m_userEdit;
    QLineEdit *m_emailEdit;
    QLineEdit *m_passEdit; // 新密码
    QLineEdit *m_codeEdit; 
    
    TimerBtn *m_getCodeBtn;
    QPushButton *m_sureBtn;
    QPushButton *m_returnBtn;
    QLabel *m_errTip;
    
    ClickedLabel *m_passVisible; // 眼睛图标

    // 错误管理
    QMap<TipErr, QString> _tip_errs;
    QMap<ReqId, std::function<void(const QJsonObject&)>> m_handlers;
};