# core/ 目录树

> 镜像主项目 apps/server/core，承载各后端服务的 Python 契约测试，并含跨服务的架构/数据库归属/注册身份等顶层契约。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`AIOrchestrator/`](AIOrchestrator/_TREE.md) | AI 编排（Python harness）服务测试 |
| [`ChatServer/`](ChatServer/_TREE.md) | ChatServer 服务结构/契约测试 |
| [`GateServer/`](GateServer/_TREE.md) | GateServer 网关结构/拆分契约测试 |
| [`VarifyServer/`](VarifyServer/_TREE.md) | VarifyServer 验证码服务契约测试 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_architecture_review_slice_d.py` | 锁定后端微服务架构评审整改项的正向不变量 |
| `test_database_ownership_contract.py` | 校验各服务数据库与应用角色归属矩阵一致 |
| `test_registration_identity_contract.py` | 校验注册/身份相关的跨服务契约 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
