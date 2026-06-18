# auth/ 目录树

> 认证功能模块，涵盖登录、注册、找回密码与凭据管理，按分层组织。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`model/`](model/_TREE.md) | 认证状态数据模型 |
| [`resources/`](resources/_TREE.md) | 认证模块 QML 资源清单 |
| [`runtime/`](runtime/_TREE.md) | 认证界面运行时 JS 逻辑 |
| [`service/`](service/_TREE.md) | 认证服务与凭据存储 |
| [`view/`](view/_TREE.md) | 登录/注册/找回密码 QML 界面 |
| [`viewmodel/`](viewmodel/_TREE.md) | 认证视图模型 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthController.cpp` | 认证控制器实现 |
| `AuthController.h` | 认证控制器声明 |
| `sources.cmake` | 汇总 auth 模块源码与资源到构建系统 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
