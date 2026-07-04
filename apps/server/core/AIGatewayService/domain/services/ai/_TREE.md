# ai/ 目录树

> AI 业务服务实现层，封装网关侧的 AI 请求处理与对编排器的调用逻辑。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | AI 公共 DTO 层的 C++ module 接口分组 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AIPublicDtos.cpp` | AI 网关公共请求与稳定响应 DTO 编解码实现 |
| `AIPublicDtos.h` | AI 网关公共请求与稳定响应 DTO 声明 |
| `AIService.cpp` | AI 业务服务实现：处理 AI 请求并对接下游能力。 |
| `AIService.h` | AI 业务服务接口声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
