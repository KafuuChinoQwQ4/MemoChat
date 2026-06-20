# async/ 目录树

> Gate 域服务的异步副作用钩子，承载登录/注册/资料变更后的审计与缓存失效等异步操作。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GateAsyncSideEffectDtos.cpp` | Gate 异步副作用 payload/envelope DTO 编解码实现 |
| `GateAsyncSideEffectDtos.h` | Gate 异步副作用 payload/envelope DTO 声明 |
| `GateAsyncSideEffects.cpp` | 异步副作用钩子实现 |
| `GateAsyncSideEffects.h` | 异步副作用钩子声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
