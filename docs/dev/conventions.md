# 开发约定

## 兼容性

MemoChat 当前不向后兼容。3.0 不兼容 2.0，开发阶段按最新协议和数据格式实现。新增或改字段时直接使用新 key，不写“读新回退旧”的双读逻辑，也不保留旧字段别名。旧客户端由强制更新机制拦截。

## 代码边界

- 服务端新逻辑优先组合和依赖注入，只有稳定抽象接口、第三方/平台适配或确需运行时多态时才引入继承。
- 服务端可抽出窄 `*.cppm` 模块承载纯算法、常量、DTO/schema 辅助；Qt/MOC、复杂第三方 ABI、I/O 胶水保留普通 `.h/.cpp`。
- QML 只表达视觉、页面组合和用户交互；业务循环放到对应 feature controller/service/viewmodel。
- React Web 按 `app / routes / features / core / shared / theme` 分层，不让 feature 互相跨导入业务实现。

## 配置和密钥

- 本地 Docker compose 可保留 `${ENV:-dev_default}` 形式的开发默认值。
- 生产/staging 密钥必须由环境变量或 Secret 注入。
- 后端密钥规则见 `apps/server/core/docs/secret-management.md`。

## 文档维护

- 新增、删除、移动文件或目录时，同步直接父目录 `_TREE.md`。
- 全项目开发文档放 `docs/`；模块细节放模块内 `docs/`。
- 一次性修复记录放 `.ai/`。
