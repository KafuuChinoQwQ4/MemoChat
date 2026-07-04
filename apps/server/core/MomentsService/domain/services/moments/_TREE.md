# moments/ 目录树

> 朋友圈业务服务：实现动态发布、点赞、评论等核心业务逻辑。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 朋友圈业务服务使用的项目自有 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `MomentsOutputDtos.cpp` | 朋友圈输出和稳定成功响应 DTO 的 row 转换与 Glaze JSON 桥接实现 |
| `MomentsOutputDtos.hpp` | 朋友圈输出和稳定成功响应 DTO 与 JsonValue 转换接口声明 |
| `MomentsPersistence.cpp` | 朋友圈本地持久化 Adapter，实现 moment 行、点赞、评论和用户资料读取 |
| `MomentsPersistence.hpp` | 朋友圈本地持久化 Interface，隔离 handler 与宽 Postgres 管理器 |
| `MomentsPublicDtos.cpp` | 朋友圈公开请求 DTO 的宽松 JSON 字段读取、兼容转换与 module-backed 发布项规范化实现 |
| `MomentsPublicDtos.hpp` | 朋友圈公开请求 DTO 和兼容解析接口声明 |
| `MomentsService.cpp` | 朋友圈业务服务实现 |
| `MomentsService.hpp` | 朋友圈业务服务声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
