# cxx_modules/ 目录树

> R18 内容源服务的 C++ module interface units，用于导出不依赖标准库实体的 source/path、DTO、编解码和官方 adapter 工具算法。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `R18AdapterUtils.cppm` | 导出 R18 adapter 工具使用的 URL 默认值、编码/转义、缓存内容类型和 Base64 primitive guard |
| `R18JmAdapter.cppm` | 导出 JM 官方 adapter 使用的源标识、API 默认值、分页、payload 和图片 URL guard |
| `R18PicacgAdapter.cppm` | 导出 Picacg 官方 adapter 使用的源/API 默认值、环境凭据读取、分页、状态、章节和图片 guard |
| `R18PublicDto.cppm` | 导出 R18 公开请求 DTO 使用的默认 source、page、favorited 与 page_index 选择算法 |
| `R18Service.cppm` | 导出 R18 service facade 使用的认证错误、导入 payload、响应状态和 content-type guard |
| `R18Source.cppm` | 导出 R18 内容源 ID、路径段和 ASCII 大小写处理算法 |
| `R18SourceService.cppm` | 导出 R18 内容源服务使用的 zip 魔数检测、搜索分页归一化、预览章节/页数、JS 探测判定和源默认字面量/错误消息 |
| `R18SourceRecordCodec.cppm` | 导出 R18 内容源记录编解码使用的默认字符串、输出指针和必填 ID 判定算法 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
