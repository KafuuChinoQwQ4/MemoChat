# controller/ 目录树

> 朋友圈控制器：按信息流、发布、评论、动态等维度拆分请求构造与响应处理。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `MomentsController.cpp` | 朋友圈控制器主实现，桥接模型与 QML |
| `MomentsController.h` | 朋友圈控制器接口声明 |
| `MomentsControllerCommentResponses.cpp` | 处理评论相关响应 |
| `MomentsControllerFeedResponses.cpp` | 处理信息流加载响应 |
| `MomentsControllerPostResponses.cpp` | 处理单条动态相关响应 |
| `MomentsControllerPublish.cpp` | 处理动态发布流程 |
| `MomentsControllerRequests.cpp` | 构造朋友圈各类请求 |
| `MomentsControllerResponses.cpp` | 响应分发的通用入口 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
