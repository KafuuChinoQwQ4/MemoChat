# cxx_modules/ 目录树

> common/runtime 的 C++ modules 接口单元，当前承载配置解析相关纯算法。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `EtcdConfig.cppm` | 导出 Etcd endpoint 拆分、响应字段探测、watch/key 边界等纯算法模块。 |
| `IniConfig.cppm` | 导出 INI 配置环境变量 key 生成使用的字符 sanitize 算法模块。 |
| `IoContextPool.cppm` | 导出 IO 上下文池大小归一化、轮询索引回绕和停止/join 守卫纯算法模块。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
