#include "userinfopage.h"
#include "ui_userinfopage.h"
#include "usermgr.h"
#include <QDebug>
#include <QMessageBox>
#include "imagecropperdialog.h"
#include "imagecropperlabel.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>

UserInfoPage::UserInfoPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserInfoPage)
{
    ui->setupUi(this);
    auto icon = UserMgr::GetInstance()->GetIcon();
    qDebug() << "icon is " << icon ;
    QPixmap pixmap(icon);
    QPixmap scaledPixmap = pixmap.scaled( ui->head_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->head_lb->setPixmap(scaledPixmap);
    ui->head_lb->setScaledContents(true);

    auto nick = UserMgr::GetInstance()->GetNick();

    auto name = UserMgr::GetInstance()->GetName();

    auto desc = UserMgr::GetInstance()->GetDesc();
    ui->nick_ed->setText(nick);
    ui->name_ed->setText(name);
    ui->desc_ed->setText(desc);
}

UserInfoPage::~UserInfoPage()
{
    delete ui;
}


void UserInfoPage::on_up_btn_clicked()
{

    QString filename = QFileDialog::getOpenFileName(
        this,
        tr("选择图片"),
        QString(),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)")
    );
    if (filename.isEmpty())
        return;


    QPixmap inputImage;
    if (!inputImage.load(filename)) {
        QMessageBox::critical(
            this,
            tr("错误"),
            tr("加载图片失败！请确认已部署 WebP 插件。"),
            QMessageBox::Ok
        );
        return;
    }

    QPixmap image = ImageCropperDialog::getCroppedImage(filename, 600, 400, CropperShape::CIRCLE);
    if (image.isNull())
        return;

    QPixmap scaledPixmap = image.scaled( ui->head_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->head_lb->setPixmap(scaledPixmap);
    ui->head_lb->setScaledContents(true);

    QString storageDir = QStandardPaths::writableLocation(
                             QStandardPaths::AppDataLocation);

    QDir dir(storageDir);
    if (!dir.exists("avatars")) {
        if (!dir.mkpath("avatars")) {
            qWarning() << "无法创建 avatars 目录：" << dir.filePath("avatars");
            QMessageBox::warning(
                this,
                tr("错误"),
                tr("无法创建存储目录，请检查权限或磁盘空间。")
            );
            return;
        }
    }

    QString filePath = dir.filePath("avatars/head.png");


    if (!scaledPixmap.save(filePath, "PNG")) {
        QMessageBox::warning(
            this,
            tr("保存失败"),
            tr("头像保存失败，请检查权限或磁盘空间。")
        );
    } else {
        qDebug() << "头像已保存到：" << filePath;

    }
}
