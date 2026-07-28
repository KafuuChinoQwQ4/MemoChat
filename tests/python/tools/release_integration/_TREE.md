# release_integration/ 目录树

> 发布入口的跨模块静态契约测试，确保客户端与后端发行 preset 保持隔离且默认安全。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_backend_deployment_kit.py` | 验证后端独立部署包的目录自洽、配置去敏、法律状态、完整迁移/provision 与 CI 归档合同 |
| `test_cpp_service_bundle.py` | 用临时 ELF/共享库验证后端服务 bundle 的依赖闭包、vcpkg SBOM、白名单与法律材料状态 |
| `test_release_preset_contract.py` | 校验 Linux 客户端/服务端发行 preset、外部法律审批公钥/签名透传、依赖源码完整性与安全默认值 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
