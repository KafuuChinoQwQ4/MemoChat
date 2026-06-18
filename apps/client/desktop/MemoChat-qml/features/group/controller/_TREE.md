# controller/ 目录树

> 群组管理控制器：拆分请求构造、事件处理、响应解析与管理效果应用等职责。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GroupController.cpp` | 群组控制器主实现，桥接管理逻辑与 QML |
| `GroupController.h` | 群组控制器接口声明 |
| `GroupManagementEffectApplier.cpp` | 将群管理结果应用到本地状态 |
| `GroupManagementEffectApplier.h` | 群管理效果应用器声明 |
| `GroupManagementEffectPorts.h` | 群管理效果依赖端口接口声明 |
| `GroupManagementEventService.cpp` | 处理服务端群管理事件 |
| `GroupManagementEventService.h` | 群管理事件服务声明 |
| `GroupManagementResponseService.cpp` | 解析群管理请求响应 |
| `GroupManagementResponseService.h` | 群管理响应服务声明 |
| `GroupRequestPayloads.cpp` | 构造群组相关请求报文 |
| `GroupRequestPayloads.h` | 群组请求报文结构声明 |
| `GroupResponseErrorService.cpp` | 群管理响应错误处理 |
| `GroupResponseErrorService.h` | 群管理错误服务声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
