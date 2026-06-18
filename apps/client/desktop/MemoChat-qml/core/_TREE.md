# core/ 目录树

> 客户端核心 C++ 层：通用工具、媒体处理、网络通信与会话数据管理，并提供构建脚本与运行时配置。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`common/`](common/_TREE.md) | 全局变量、枚举、单例模板等通用基础设施 |
| [`media/`](media/_TREE.md) | 图片裁剪、屏幕截图、摄像头采集等媒体处理 |
| [`network/`](network/_TREE.md) | HTTP/TCP/QUIC 传输与聊天消息编解码、分发 |
| [`session/`](session/_TREE.md) | 用户、好友、群组、消息等会话数据与管理 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | MemoChatCore 模块的构建脚本（C++ 标准、AUTOMOC/RCC 等） |
| `config.ini` | 客户端运行时配置（GateServer 地址、媒体限制、协议回退等） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
