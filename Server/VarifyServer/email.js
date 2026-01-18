const nodemailer = require('nodemailer');
const config_module = require("./config");

/**
 * 创建发送邮件的代理
 * 这里以 163 为例，如果是 Gmail，host 改为 smtp.gmail.com
 */
let transport = nodemailer.createTransport({
    host: 'smtp.qq.com', 
    port: 465,
    secure: true,
    auth: {
        user: config_module.email_user,
        pass: config_module.email_pass
    }
});

function SendMail(mailOptions_){
    return new Promise(function(resolve, reject){
        transport.sendMail(mailOptions_, function(error, info){
            if (error) {
                console.log(error);
                reject(error);
            } else {
                console.log('邮件已成功发送：' + info.response);
                resolve(info.response)
            }
        });
    })
}

module.exports.SendMail = SendMail;