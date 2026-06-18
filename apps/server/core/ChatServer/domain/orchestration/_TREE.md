# orchestration/ 目录树

> 编排层：逻辑系统（消息分派调度核心）、运行时对象图装配与处理器注册，把各类请求绑定到对应领域服务处理函数。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatHandlerRegistrars.cpp` | 各类消息处理器注册实现 |
| `ChatHandlerRegistrars.h` | 处理器注册声明 |
| `ChatRuntimeComposition.cpp` | ChatServer 运行时对象图装配实现 |
| `ChatRuntimeComposition.h` | ChatServer 运行时对象图装配声明 |
| `LogicSystem.cpp` | 逻辑系统（请求分派调度）实现 |
| `LogicSystem.h` | 逻辑系统声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
