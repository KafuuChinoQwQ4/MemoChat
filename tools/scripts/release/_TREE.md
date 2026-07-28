# release/ 目录树

> Linux 客户端与 C++ 后端的可复现发布构建、打包、镜像和敏感信息校验工具。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `audit_backend_images.sh` | 按固定 15 服务映射校验镜像身份、非 root 元数据与 digest/provenance 绑定，并生成含 Grype 数据库状态的 SBOM 和漏洞证据 |
| `build_backend_images.sh` | 验证含 commit-bound vcpkg SBOM 的 15 个后端 service bundle，并以固定 Ubuntu digest（默认离线复用本地层）通过隔离的最小命名上下文构建运行时镜像 |
| `client_release_scan.allowlist` | 客户端发布扫描中允许出现公开认证文案的精确相对路径 |
| `generate_vcpkg_installed_sbom.py` | 从指定 triplet 的 vcpkg status 与 SPDX 记录生成绑定 source SHA 的确定性依赖闭包 SBOM |
| `load_build_environment.sh` | 校验并加载私有构建环境，仅保留工具链变量，清除运行时密钥和会覆盖受审计 preset 的并发参数 |
| `package_backend_deployment_kit.sh` | 生成含固定 Compose、去敏配置、迁移/provision 与 exact-source 外部法律审批证据的独立后端部署包 |
| `package_backend_services.sh` | 在清洁动态加载环境与显式库根边界内为 15 个 C++ 服务生成绑定外部法律审批摘要和 vcpkg SBOM 的依赖闭包 bundle |
| `package_linux_client.sh` | 生成含 exact-source 外部法律审批状态的便携 Linux 客户端、可选嵌入部署 CA，并产出归档、清单和校验和 |
| `run_release_compose.sh` | 按所选 profile 校验私有运行环境、密钥和挂载权限后执行固定的后端 release Compose |
| `verify_release_legal.sh` | 生成并验证绑定 source SHA/tree、根法律文件和 corpus 摘要的仓库外 v2 审批载荷与签名，阻止自声明、重放或不完整版本发布 |
| `verify_release_tree.sh` | 拒绝发布树中的密钥、运行时状态、开发路径、危险链接和缺失 ELF 依赖，并识别公开 env 示例与 schema 文件边界 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
