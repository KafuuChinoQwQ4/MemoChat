# cxx_modules/ 目录树

> MediaService 媒体业务 DTO 层的 C++ module interface units，用于导出不依赖标准库实体的纯算法。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `MediaPublicDto.cppm` | 导出媒体公共上传 DTO 使用的默认 media_type 与 JSON content-type ASCII 判断算法 |
| `MediaService.cppm` | 导出媒体 service facade 使用的响应元数据、上传参数、分片会话与下载 guard 算法 |
| `MediaUploadSession.cppm` | 导出媒体分片上传会话 DTO 使用的默认值与基础有效性判断算法 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
