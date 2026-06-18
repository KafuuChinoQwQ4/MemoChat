# h2/ 目录树

> HTTP/2 传输实现：适配器、各业务处理器、路由、支撑工具与可独立运行的 standalone 程序。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`adapters/`](adapters/_TREE.md) | H2 到统一路由契约的适配器。 |
| [`handlers/`](handlers/_TREE.md) | 各业务 H2 请求处理器。 |
| [`routes/`](routes/_TREE.md) | H2 路由表装配。 |
| [`standalone/`](standalone/_TREE.md) | 可独立运行的 H2 网关程序。 |
| [`support/`](support/_TREE.md) | H2 各业务的支撑工具。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | H2 传输构建目标定义。 |
| `config.ini` | H2 运行时配置。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
