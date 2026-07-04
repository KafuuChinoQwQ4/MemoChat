# cxx_modules/ 目录树

> MediaService 存储层的 C++ module interface units，用于导出不依赖标准库实体的纯算法。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `LocalMediaStorage.cppm` | 导出本地媒体存储使用的默认目录/内容类型、文件名净化字符策略、HTTP URL 与存储后端判断算法 |
| `S3MediaStorage.cppm` | 导出 S3/MinIO 存储使用的默认区域/桶/内容类型、对象名净化字符策略、启用判断与桶前缀判断算法 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
