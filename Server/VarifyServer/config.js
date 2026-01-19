const fs = require('fs');

let config = JSON.parse(fs.readFileSync('config.json', 'utf8'));
let email_user = config.email.user;
let email_pass = config.email.pass;

// === 新增 Redis 配置读取 ===
let redis_host = config.redis.host;
let redis_port = config.redis.port;
let redis_passwd = config.redis.passwd;

module.exports = {
    email_user, 
    email_pass,
    redis_host,
    redis_port,
    redis_passwd
};