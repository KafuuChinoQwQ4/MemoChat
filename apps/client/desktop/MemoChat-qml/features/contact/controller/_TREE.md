# controller/ 目录树

> 联系人控制器：协调好友增删改查、申请审批与服务端事件分发。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ContactController.cpp` | 联系人控制器主实现，桥接模型与 QML |
| `ContactController.h` | 联系人控制器接口声明 |
| `ContactEventService.cpp` | 处理服务端推送的联系人事件 |
| `ContactEventService.h` | 联系人事件服务接口声明 |
| `ContactRequestPayloads.cpp` | 构造联系人相关请求报文 |
| `ContactRequestPayloads.h` | 联系人请求报文结构声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
