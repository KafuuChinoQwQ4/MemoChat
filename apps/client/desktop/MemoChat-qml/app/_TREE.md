# app/ 目录树

> 应用装配层，负责启动引导、依赖组合与端口/信号绑定、连接与会话协调、控制器装配，把核心运行时与功能模块拼装成可运行的客户端。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`bootstrap/`](bootstrap/_TREE.md) | 进程启动引导：日志、平台、QML 引擎与运行时配置 |
| [`composition/`](composition/_TREE.md) | 依赖组合根，注册功能模块并绑定端口与信号 |
| [`connection/`](connection/_TREE.md) | 聊天连接协调与连接策略 |
| [`controller/`](controller/_TREE.md) | 应用主控制器及其各业务状态/绑定切片 |
| [`coordinators/`](coordinators/_TREE.md) | 通话、媒体、注册倒计时等跨模块协调器 |
| [`events/`](events/_TREE.md) | 聊天分发与 HTTP 事件路由 |
| [`session/`](session/_TREE.md) | 会话与鉴权协调，登录后关系引导 |
| [`shell/`](shell/_TREE.md) | 外壳状态控制与外壳视图模型 |
| [`window/`](window/_TREE.md) | 主窗口效果与平台窗口钩子 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `sources.cmake` | app 层源码清单，供主工程构建引用 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
