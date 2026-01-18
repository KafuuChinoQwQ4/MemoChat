const path = require('path')
const grpc = require('@grpc/grpc-js')
const protoLoader = require('@grpc/proto-loader')
const emailModule = require('./email')
const const_module = require('./const')
const { v4: uuidv4 } = require('uuid'); // 引入 uuid

// 1. 加载 Proto
const PROTO_PATH = path.join(__dirname, 'message.proto')
const packageDefinition = protoLoader.loadSync(PROTO_PATH, { 
    keepCase: true, 
    longs: String, 
    enums: String, 
    defaults: true, 
    oneofs: true 
})
const protoDescriptor = grpc.loadPackageDefinition(packageDefinition)
const message_proto = protoDescriptor.message

// 2. 业务逻辑：获取验证码
async function GetVarifyCode(call, callback) {
    console.log("Receive request for email:", call.request.email)
    try {
        let uniqueId = uuidv4().substring(0, 6); // 生成6位随机码
        let text_str = '您的验证码为 ' + uniqueId + '，请在三分钟内完成注册。'
        
        // 发送邮件
        let mailOptions = {
            from: 'kafu_chino@qq.com', // ⚠️ 这里要改成和你 config.json 里一样的发件人
            to: call.request.email,
            subject: 'MemoChat 注册验证码',
            text: text_str,
        };
    
        let send_res = await emailModule.SendMail(mailOptions);
        console.log("Send result:", send_res)

        // 回复 gRPC 成功
        callback(null, { 
            email: call.request.email,
            error: const_module.Errors.Success,
            code: uniqueId // 把生成的验证码返给 GateServer (调试用)
        }); 

    } catch(error) {
        console.log("Error:", error)
        callback(null, { 
            email:  call.request.email,
            error: const_module.Errors.Exception
        }); 
    }
}

// 3. 启动服务
function main() {
    var server = new grpc.Server()
    server.addService(message_proto.VarifyService.service, { GetVarifyCode: GetVarifyCode })
    
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        console.log('VarifyServer (Node.js) started on port 50051')        
    })
}

main()