# release/ 目录树

> Linux 客户端与 C++ 后端的可复现发布构建、打包、镜像和敏感信息校验工具。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `audit_backend_images.sh` | 按固定 15 服务映射校验镜像身份、非 root 元数据与 digest/provenance 绑定，并生成含 Grype 数据库状态的 SBOM 和漏洞证据 |
| `build_backend_images.sh` | 验证含 commit-bound vcpkg SBOM 的 15 个后端 service bundle，并以固定 Ubuntu digest、可选仅构建期 CA secret 和隔离的最小命名上下文构建运行时镜像 |
| `client_release_scan.allowlist` | 客户端发布扫描中允许出现公开认证文案的精确相对路径 |
| `compute_release_source_snapshot.py` | 对指定 Git 提交的完整 tracked tree 计算确定性 SHA-256，并排除自引用的 `legal/third-party` corpus |
| `generate_vcpkg_installed_sbom.py` | 从指定 triplet 的 vcpkg status 与 SPDX 记录生成绑定 source SHA 的确定性依赖闭包 SBOM |
| `load_build_environment.sh` | 校验并加载私有构建环境，仅保留工具链变量，清除运行时密钥和会覆盖受审计 preset 的并发参数 |
| `package_backend_deployment_kit.sh` | 生成含固定 Compose、去敏配置、迁移/provision 与 exact-source 第三方材料状态的独立后端部署包 |
| `package_backend_services.sh` | 在清洁动态加载环境与显式库根边界内为 15 个 C++ 服务生成绑定第三方材料摘要和 vcpkg SBOM 的依赖闭包 bundle |
| `package_linux_client.sh` | 生成含 exact-source 第三方材料状态的便携 Linux 客户端、可选嵌入部署 CA，并产出归档、清单和校验和 |
| `run_release_compose.sh` | 按所选 profile 校验私有运行环境、密钥和挂载权限后执行固定的后端 release Compose |
| `verify_release_legal.sh` | 校验根法律文件、完整第三方 corpus、校验和与 clean source SHA/tree 绑定，阻止材料不完整或源码漂移的版本发布 |
| `verify_release_tree.sh` | 拒绝发布树中的密钥、运行时状态、开发路径、危险链接和缺失 ELF 依赖，并识别公开 env 示例与 schema 文件边界 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
