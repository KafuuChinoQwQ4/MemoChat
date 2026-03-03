const fs = require('fs');
const path = require('path');

const configPath = path.join(__dirname, 'config.json');
const config = JSON.parse(fs.readFileSync(configPath, 'utf8'));

const email_host = config.email.host || 'smtp.163.com';
const email_port = config.email.port || 465;
const email_secure = typeof config.email.secure === 'boolean' ? config.email.secure : true;
const email_user = config.email.user;
const email_pass = config.email.pass;
const email_from = config.email.from || email_user;
const mysql_host = config.mysql.host;
const mysql_port = config.mysql.port;
const redis_host = config.redis.host;
const redis_port = config.redis.port;
const redis_passwd = config.redis.passwd;
const code_prefix = 'code_';

const log_level = (config.log && config.log.level) || 'info';
const log_dir = (config.log && config.log.dir) || './logs';
const log_to_console = config.log ? config.log.toConsole !== false : true;
const log_env = (config.log && config.log.env) || 'local';

module.exports = {
  email_host,
  email_port,
  email_secure,
  email_user,
  email_pass,
  email_from,
  mysql_host,
  mysql_port,
  redis_host,
  redis_port,
  redis_passwd,
  code_prefix,
  log_level,
  log_dir,
  log_to_console,
  log_env,
};
