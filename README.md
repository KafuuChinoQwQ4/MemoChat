# MemoChat

该项目基于 C++23/26 与 Qt6.8.2/Qml 的新标准试验田，主要测试各个新标准和新第三方库，不具备长期维护性，新实现会直接覆盖旧实现，不具备向前兼容，且该项目内容较繁杂，多个无关功能堆叠，但后端以具备微服务骨架，能较好拆分模块，未来可能会直接从该项目直接抽出某一模块功能进行长期维护。

项目采用Web端采用 React 框架，通过 WebSocket 与 WebTransport 适配 C++26 为基准的后端，桌面应用采用 Qt6.8.2 长期维护版本实现，Qml 作为 UI 设计标准，TypeScript 作为局部简单逻辑实现，C++ 作为全局调控逻辑。

整体项目使用 Cmake4.3.3 Ninja1.13.2 构建。（构建系统版本过低会导致 Cpp modules 功能构建时无缓存，会反复建立，建议升级版本）

目前该项目有正常 IM 服务，音视频功能采用 GitHub 现成方案。（未来会以 WebRTC 重构）

Live2D AI 化操纵，通过 OpenCV 与 GPT-SoVIST 进行视觉识别和语音模拟，接入 Agent。

## Linux 发布前置条件

Linux 客户端与 C++ 后端发布任务运行在项目私有的 self-hosted runner。该 runner 需要提供
`/root/.memochat-linux-env`，文件所有者必须是运行 Actions 的当前用户，权限必须精确为
`0600`。文件仅允许 shell 变量赋值和简单的 `unset`；CI 通过
`tools/scripts/release/load_build_environment.sh` 加载后，只保留 PATH、CMake、Qt、编译器和
vcpkg 等工具链变量，并在构建进程启动前移除 `MEMOCHAT_*` 及其他密码、令牌和云凭据变量。

可在 runner 上检查前置条件：

```bash
test "$(stat -c '%u' /root/.memochat-linux-env)" = "$(id -u)"
test "$(stat -c '%a' /root/.memochat-linux-env)" = "600"
source tools/scripts/release/load_build_environment.sh /root/.memochat-linux-env
test -x "$VCPKG_ROOT/vcpkg"
```

推送 `v*` 标签会发布 Linux 客户端/后端压缩包、SHA-256 校验文件、发布清单和 GHCR
版本镜像别名。正式标签发布前，项目所有者必须在仓库根目录提供非空的常规文件 `LICENSE`
和 `THIRD_PARTY_NOTICES.md`；缺少任一文件时 CI 会在构建和发布前停止。普通分支和 PR
不受该法律文件门禁影响，外部 fork PR 不会进入私有 self-hosted runner。版本标签必须是
位于 `main` 历史上的 annotated SemVer 标签；发布前 CI 会扫描完整 Git 历史，已经存在的
GitHub Release 和同版本资产不允许覆盖。

本地发布部署入口、私有环境文件格式、数据服务初始化、可选服务 profile 和客户端本地
CA 打包方式见 [`infra/deploy/local/README.md`](infra/deploy/local/README.md)。

Linux 发布目标为 x86_64 Ubuntu 24.04。客户端压缩包的 `RELEASE-INFO.txt` 会根据最终 Qt 与
共享库闭包记录精确的最低 glibc 版本，其他发行版必须提供不低于该版本的 glibc；C++ 后端
通过固定 digest 的 Ubuntu 24.04 runtime image 运行，宿主机只需满足 Docker/Compose 要求。
