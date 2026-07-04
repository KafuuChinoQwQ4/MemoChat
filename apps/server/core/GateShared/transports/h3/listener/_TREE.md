# listener/ 目录树

> HTTP/3（QUIC）的连接处理、JSON 支持与监听器实现。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`modules/`](cxx_modules/GateHttp3JsonSupport.cppm) | H3 JSON POST 支持 guard/状态码/content-type/错误字面量算法 module |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GateHttp3Connection.cpp` | H3 连接处理与响应状态/header/body 存储实现。 |
| `GateHttp3Connection.h` | H3 连接接口声明，暴露响应状态/header/body 查询。 |
| `GateHttp3JsonSupport.cpp` | H3 JSON 解析/构造支持实现，导入算法 module 处理 guard、状态码与错误字面量。 |
| `GateHttp3JsonSupport.h` | H3 JSON 支持声明。 |
| `GateHttp3Listener.cpp` | H3 监听器实现，负责组装响应状态、header 和 body。 |
| `GateHttp3Listener.h` | H3 监听器接口声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
