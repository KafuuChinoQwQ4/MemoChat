# release_security/ 目录树

> 发布安全合同测试：验证干净暂存、制品敏感信息扫描、配置日志脱敏、Action 锁定与不可变 CI/CD 产物交接。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_backend_image_audit.py` | 覆盖 15 个后端镜像的服务身份、digest/法律/SBOM 绑定、敏感环境及完整 Grype 报告失败关闭合同 |
| `test_release_legal.py` | 验证法律清单与正式第三方语料分离、仓库外 v2 审批载荷/签名、精确 source tree 防重放及正式分发失败关闭合同 |
| `test_release_security_contract.py` | 覆盖发布树扫描、Action SHA、仅标签发布、source SHA、显式构建上下文和 GHCR 内容绑定工作流合同 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
