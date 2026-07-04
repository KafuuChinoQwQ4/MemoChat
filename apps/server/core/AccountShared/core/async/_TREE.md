# async/ 目录树

> Gate 域服务的异步副作用钩子，承载登录/注册/资料变更后的审计与缓存失效等异步操作。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 异步副作用 DTO 使用的项目自有 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GateAsyncSideEffectDtos.cpp` | Gate 异步副作用 payload/envelope DTO 编解码实现 |
| `GateAsyncSideEffectDtos.h` | Gate 异步副作用 payload/envelope DTO 声明 |
| `GateAsyncSideEffects.cpp` | 异步副作用钩子实现 |
| `GateAsyncSideEffects.h` | 异步副作用钩子声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
