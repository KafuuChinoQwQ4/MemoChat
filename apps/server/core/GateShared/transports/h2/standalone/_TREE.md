# standalone/ 目录树

> 可独立运行的 HTTP/2 网关程序，自带证书工具、应用骨架与入口。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CertUtil.cpp` | TLS 证书工具实现。 |
| `CertUtil.h` | 证书工具声明。 |
| `GateServerHttp2.cpp` | H2 网关服务器实现。 |
| `GateServerHttp2.h` | H2 网关服务器声明。 |
| `Http2App.cpp` | H2 应用骨架实现。 |
| `Http2App.h` | H2 应用骨架声明。 |
| `WinCompat.h` | Windows 兼容性垫片。 |
| `main.cpp` | 独立 H2 网关进程入口。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
