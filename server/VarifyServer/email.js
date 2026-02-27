const nodemailer = require('nodemailer');
const configModule = require('./config');

const transport = nodemailer.createTransport({
    host: configModule.email_host,
    port: configModule.email_port,
    secure: configModule.email_secure,
    auth: {
        user: configModule.email_user,
        pass: configModule.email_pass
    }
});

function SendMail(mailOptions) {
    return new Promise((resolve, reject) => {
        transport.sendMail(mailOptions, (error, info) => {
            if (error) {
                console.log(error);
                reject(error);
                return;
            }
            console.log('mail sent: ' + info.response);
            resolve(info.response);
        });
    });
}

module.exports.SendMail = SendMail;
