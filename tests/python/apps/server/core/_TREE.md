# core/ 目录树

> 镜像主项目 apps/server/core，承载各后端服务的 Python 契约测试，并含跨服务的架构/数据库归属/注册身份等顶层契约。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`AIGatewayService/`](AIGatewayService/_TREE.md) | AIGatewayService AI 网关配置契约测试 |
| [`AIOrchestrator/`](AIOrchestrator/_TREE.md) | AI 编排（Python harness）服务测试 |
| [`ChatServer/`](ChatServer/_TREE.md) | ChatServer 服务结构/契约测试 |
| [`GateServer/`](GateServer/_TREE.md) | GateServer 网关结构/拆分契约测试 |
| [`VarifyServer/`](VarifyServer/_TREE.md) | VarifyServer 验证码服务契约测试 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_architecture_review_slice_d.py` | 锁定后端微服务架构评审整改项的正向不变量 |
| `test_database_ownership_contract.py` | 校验各服务数据库与应用角色归属矩阵一致 |
| `test_cxx_modules_contract.py` | 校验服务端 GNU C++ modules 自定义 CMI 构建契约 |
| `test_r18_source_pagination_contract.py` | 校验 R18 漫画源分页契约，防止满页结果阻断后续加载 |
| `test_focused_persistence_seams.py` | 校验 focused service handler 只能通过本地持久化 Interface 访问 Postgres |
| `test_registration_identity_contract.py` | 校验注册/身份相关的跨服务契约 |
| `test_r18_gate_header_security_contract.py` | 校验 R18 上游 TLS/ECB 边界与 Gate HTTP header 注入防护契约 |
| `test_security_hardening_contract.py` | 校验认证安全加固契约：登录限流与密钥注入防呆 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
