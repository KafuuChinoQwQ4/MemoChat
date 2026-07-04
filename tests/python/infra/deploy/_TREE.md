# deploy/ 目录树

> 镜像 infra/deploy 部署编排，承载部署相关契约测试。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`kubernetes/`](kubernetes/_TREE.md) | Kubernetes/Helm Chart 契约测试 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_local_envoy_security_contract.py` | 校验本地 Envoy 传输安全、HSTS 与认证限流配置 |
| `test_secret_externalization_contract.py` | 校验本地部署、AI 栈与服务配置中的密钥外置化契约 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
