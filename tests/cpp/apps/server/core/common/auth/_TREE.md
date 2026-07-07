# auth/ 目录树

> 测试服务端公共认证基础库，覆盖 JWT access token 等可独立验证的认证算法。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册公共认证算法 GTest 目标 |
| `JwtAccessTokenTest.cpp` | 验证 JWT access token 签发、验签、过期与 claim 失败路径 |
| `main.cpp` | 公共认证 GTest 入口 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
