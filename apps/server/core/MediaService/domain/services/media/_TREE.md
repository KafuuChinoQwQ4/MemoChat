# media/ 目录树

> 媒体业务服务：实现媒体资产的核心业务逻辑。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 媒体公共 DTO 层的 C++ module 接口与低依赖算法 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `MediaPersistence.cpp` | 媒体资产元数据与下载授权 grant 的本地持久化 Adapter |
| `MediaPersistence.hpp` | 媒体资产元数据与下载授权 grant 的本地持久化 Interface，隔离 handler 与宽 Postgres 管理器 |
| `MediaPublicDtos.cpp` | 媒体公共上传请求与固定成功响应 DTO 编解码实现 |
| `MediaPublicDtos.hpp` | 媒体公共上传请求与固定成功响应 DTO 声明 |
| `MediaService.cpp` | 媒体业务服务实现 |
| `MediaService.hpp` | 媒体业务服务声明 |
| `MediaUploadSessionDto.cpp` | 媒体分片上传会话 JSON DTO 编解码实现 |
| `MediaUploadSessionDto.hpp` | 媒体分片上传会话 JSON DTO 声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
