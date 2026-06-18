# core/ 目录树

> 服务端微服务源码树根：GateServer 拆分后的各域服务（Account/Login/Register/Call/AI/Media/Moments/R18）、ChatServer 聊天微服务集群、验证码服务，以及它们共享的 common/ 基础库与 docs/ 设计文档。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`AIGatewayService/`](AIGatewayService/_TREE.md) | AI 网关服务，对外暴露 AI 聊天/会话接口 |
| [`AIOrchestrator/`](AIOrchestrator/_TREE.md) | AI 编排服务，串联模型调用与知识库流程 |
| [`AIServer/`](AIServer/_TREE.md) | AI 后端服务，承载模型推理与流式响应 |
| [`AccountService/`](AccountService/_TREE.md) | memo_account 数据 owner 服务（D-ACCOUNT），承载资料写入 |
| [`AccountShared/`](AccountShared/_TREE.md) | Account/Login/Register 共享的领域逻辑与持久化库 |
| [`CallService/`](CallService/_TREE.md) | 通话信令域服务，从 Gate 剥离的 /api/call/* |
| [`ChatServer/`](ChatServer/_TREE.md) | 分层聊天微服务集群（接入/消息/关系/投递） |
| [`GateShared/`](GateShared/_TREE.md) | 各域服务复用的 Gate 框架与运行骨架 |
| [`LoginService/`](LoginService/_TREE.md) | 登录鉴权服务，签发会话 token 与登录票据 |
| [`MediaService/`](MediaService/_TREE.md) | 媒体上传/访问控制服务 |
| [`MomentsService/`](MomentsService/_TREE.md) | 朋友圈/动态域服务 |
| [`R18Service/`](R18Service/_TREE.md) | R18 内容域服务 |
| [`RegisterService/`](RegisterService/_TREE.md) | 注册与找回密码服务 |
| [`VarifyServer/`](VarifyServer/_TREE.md) | 验证码生成与邮件下发服务 |
| [`common/`](common/_TREE.md) | 跨服务共享基础库（日志/集群发现/proto/运行时等） |
| [`docs/`](docs/_TREE.md) | 服务端设计与归属文档 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | core 源码树的 CMake 聚合构建脚本 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
