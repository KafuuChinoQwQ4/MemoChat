# gateserver/ 目录树

> GateServer 及相关服务的启动/运行/监控脚本（Windows 为主），含选择二进制、开发态启动与缺失服务补启。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `run_gateserver.bat` | 启动 GateServer 的批处理脚本 |
| `run_gateserver.ps1` | 启动 GateServer 的 PowerShell 脚本 |
| `run_gateserver_debug.ps1` | 以调试模式启动 GateServer |
| `select_server_bins.ps1` | 选择/切换使用的服务端二进制 |
| `start-windows-dev.ps1` | Windows 开发态一键启动脚本 |
| `start_and_monitor.ps1` | 启动并持续监控 GateServer |
| `start_gateserver.bat` | 启动 GateServer 的批处理脚本（变体） |
| `start_missing_services.bat` | 补启尚未运行的依赖服务 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
