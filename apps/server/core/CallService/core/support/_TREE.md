# support/ 目录树

> 通话核心服务实现，处理通话会话、LiveKit token 与 Redis 状态等业务。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 通话核心服务使用的项目自有 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CallPersistence.cpp` | 通话本地持久化 Adapter，实现会话、好友关系和用户资料访问 |
| `CallPersistence.hpp` | 通话本地持久化 Interface，隔离核心服务与宽 Postgres 管理器 |
| `CallPublicDtos.cpp` | 通话公共请求与固定成功响应 DTO 编解码实现 |
| `CallPublicDtos.hpp` | 通话公共请求与固定成功响应 DTO 声明 |
| `CallSessionCacheDto.cpp` | 通话会话 Redis 缓存 DTO 编解码实现 |
| `CallSessionCacheDto.hpp` | 通话会话 Redis 缓存 DTO 声明 |
| `CallService.cpp` | 通话核心服务实现 |
| `CallService.hpp` | 通话核心服务声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
