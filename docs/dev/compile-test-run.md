# 构建、测试与运行

## C++/QML 全量构建

Linux 默认构建路径：

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
ctest --preset linux-full-gcc16 --output-on-failure
```

顶层 CMake 入口会按开关构建：

- `BUILD_SERVER`：服务端目标。
- `BUILD_CLIENT`：`MemoChatQml` 桌面客户端。
- `BUILD_OPS`：MemoOps QML。
- `BUILD_TESTS`：根目录 `tests/` 下的测试。

## Web 前端

```bash
cd apps/web
npm run type-check
npm run lint
npm run test
npm run build
```

与浏览器聊天链路相关的 smoke 脚本在 `apps/web/package.json` 中：

- `npm run smoke:websocket`
- `npm run smoke:webtransport`
- `npm run smoke:cross-transport`

## 测试放置

新增持久测试统一放根目录 `tests/`：

- C++ GTest：`tests/cpp/<主项目相对路径>/...`
- Python：`tests/python/<主项目相对路径>/...`
- fixtures：`tests/fixtures/...`

不要在业务模块下新增新的测试目录。历史测试可以读取，但迁移和新增按根目录规则执行。

## 文档改动验证

文档改动通常不需要运行全量构建。至少执行：

```bash
git diff --check
```

如果改了 `_TREE.md`，还要确认链接目标存在、父目录表格覆盖新增/删除的直接子项。
