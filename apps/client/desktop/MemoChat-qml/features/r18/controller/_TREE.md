# controller/ 目录树

> R18 消费端控制器：按服务端成人授权、网络收发、响应解析、只读源选择和阅读状态维护拆分。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `R18Controller.cpp` | R18Controller 主体：构造、成人授权查询/确认与源/漫画刷新等入口逻辑 |
| `R18Controller.h` | R18Controller 声明：暴露服务端访问策略、各列表模型及阅读状态的 Q_PROPERTY |
| `R18ControllerNetwork.cpp` | 网络请求发送与对接网关的实现 |
| `R18ControllerPrivate.h` | 内部辅助（请求超时、SSL 配置、路径判断等）私有声明 |
| `R18ControllerResponses.cpp` | 解析网关授权与内容 JSON 并分发到各模型/状态 |
| `R18ControllerSources.cpp` | 只读漫画源目录加载与本地目录路径选择逻辑，不暴露全局源变更入口 |
| `R18ControllerState.cpp` | access/loading/error/官方源目录等内部状态的 setter |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
