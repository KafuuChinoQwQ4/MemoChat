# runtime/ 目录树

> 服务运行时基础设施：Etcd/INI 配置加载、IO 上下文线程池与单例模板。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `EtcdConfig.cpp` | 从 Etcd 拉取运行时配置实现 |
| `EtcdConfig.h` | Etcd 配置声明 |
| `IniConfig.cpp` | INI 文件配置解析实现 |
| `IniConfig.h` | INI 配置声明 |
| `IoContextPool.cpp` | Asio IO 上下文线程池实现 |
| `IoContextPool.h` | IO 上下文池声明 |
| `Singleton.h` | 通用单例模板 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
