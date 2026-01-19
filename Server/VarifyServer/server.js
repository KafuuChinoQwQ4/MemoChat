const path = require('path')
const grpc = require('@grpc/grpc-js')
const protoLoader = require('@grpc/proto-loader')
const emailModule = require('./email')
const const_module = require('./const')
const { v4: uuidv4 } = require('uuid');
const redis_module = require('./redis') // <--- 1. 引入 Redis 模块

// 加载 Proto (保持不变)
const PROTO_PATH = path.join(__dirname, 'message.proto')
const packageDefinition = protoLoader.loadSync(PROTO_PATH, { 
    keepCase: true, longs: String, enums: String, defaults: true, oneofs: true 
})
const protoDescriptor = grpc.loadPackageDefinition(packageDefinition)
const message_proto = protoDescriptor.message

// 2. 业务逻辑：获取验证码 (核心修改)
async function GetVarifyCode(call, callback) {
    console.log("email is ", call.request.email)
    try {
        // === Step A: 先查 Redis ===
        let query_res = await redis_module.GetRedis(const_module.code_prefix + call.request.email);
        console.log("query_res is ", query_res)
        
        let uniqueId = query_res;
        
        // === Step B: 如果 Redis 里没有，才生成新的 ===
        if (query_res == null) {
            uniqueId = uuidv4();
            if (uniqueId.length > 4) {
                uniqueId = uniqueId.substring(0, 4); // 截取4位
            } 
            // 写入 Redis，设置过期时间 600秒 (10分钟)
            let bres = await redis_module.SetRedisExpire(const_module.code_prefix + call.request.email, uniqueId, 600)
            if(!bres){
                callback(null, { 
                    email: call.request.email,
                    error: const_module.Errors.RedisErr
                });
                return;
            }
        }

        console.log("uniqueId is ", uniqueId)
        let text_str = '您的验证码为 ' + uniqueId + ' 请在三分钟内完成注册'
        
        // === Step C: 发送邮件 ===
        // (注意：这里逻辑上其实如果是缓存的验证码，通常不发邮件或限制发送频率，
        // 但教程代码里即使是查出来的也发邮件，我们先按教程写)
        let mailOptions = {
            from: 'kafu_chino@qq.com', // 确保和你 config.json 一致
            to: call.request.email,
            subject: '验证码',
            text: text_str,
        };
    
        let send_res = await emailModule.SendMail(mailOptions);
        console.log("send res is ", send_res)

        callback(null, { 
            email: call.request.email,
            error: const_module.Errors.Success,
            code: uniqueId
        }); 
        
    } catch(error) {
        console.log("catch error is ", error)
        callback(null, { 
            email: call.request.email,
            error: const_module.Errors.Exception
        }); 
    }
}

// 启动服务 (保持不变)
function main() {
    var server = new grpc.Server()
    server.addService(message_proto.VarifyService.service, { GetVarifyCode: GetVarifyCode })
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        console.log('VarifyServer (Node.js) started on port 50051')        
    })
}

main()