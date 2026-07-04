# relation/ 目录树

> 关系领域：好友关系服务、内部关系 gRPC、关系-会话适配器，以及通过工厂装配关系命令/查询服务。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 关系领域可导入的窄 C++ module 算法接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatRelationInternalGrpcService.cpp` | 内部关系 gRPC 服务实现 |
| `ChatRelationInternalGrpcService.h` | 内部关系 gRPC 服务声明 |
| `ChatRelationService.cpp` | 好友关系服务实现 |
| `ChatRelationService.h` | 好友关系服务声明 |
| `ChatRelationSessionAdapter.cpp` | 关系-会话适配器实现 |
| `ChatRelationSessionAdapter.h` | 关系-会话适配器声明 |
| `RelationServiceFactory.cpp` | 关系服务工厂实现 |
| `RelationServiceFactory.h` | 关系服务工厂声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
