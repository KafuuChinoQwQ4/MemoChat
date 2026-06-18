# controller/ 目录树

> R18 控制器：按网络收发、响应解析、源管理、状态维护拆分的多文件 C++ 实现。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `R18Controller.cpp` | R18Controller 主体：构造与源/漫画刷新等入口逻辑 |
| `R18Controller.h` | R18Controller 声明：暴露各列表模型及阅读状态的 Q_PROPERTY |
| `R18ControllerNetwork.cpp` | 网络请求发送与对接网关的实现 |
| `R18ControllerPrivate.h` | 内部辅助（请求超时、SSL 配置、路径判断等）私有声明 |
| `R18ControllerResponses.cpp` | 解析网关返回 JSON 并分发到各模型/状态 |
| `R18ControllerSources.cpp` | 漫画源的导入、本地文件选取与源管理逻辑 |
| `R18ControllerState.cpp` | loading/error/官方源目录等内部状态的 setter |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
