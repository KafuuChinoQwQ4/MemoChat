# storage/ 目录树

> 媒体存储抽象层：定义存储接口并提供 S3/对象存储后端实现。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `MediaStorage.cpp` | 媒体存储抽象接口、本地目录写入预检及所选后端启动/关闭生命周期实现 |
| `MediaStorage.hpp` | 带 Ready/StartupError 启动契约的媒体存储抽象接口声明 |
| `S3MediaStorage.cpp` | 基于 S3/MinIO 的存储实现及启用、凭据、端点和桶可达性预检 |
| `S3MediaStorage.hpp` | 带显式启动状态的 S3 媒体存储实现声明 |

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 存储层的 C++ module interface units |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
