# backend_release/ 目录树

> 校验 C++ 后端发布镜像、入口脚本、本地 Compose 与环境变量示例的安全合同。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_backend_release_contract.py` | 覆盖服务 bundle、入口 fail-closed、完整拓扑与密钥外置化合同。 |
| `test_postgres_legacy_split_migration.py` | 用临时 Postgres 验证 split copy marker、失败原子重试、不回灌及 calls 数据库的条件化 provision。 |
| `test_release_compose_preflight.py` | 验证 release Compose 启动前的私有环境、按 profile 的密钥强度、挂载权限与固定入口合同。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
