# RegisterService/ 目录树

> 注册与找回密码服务，从 GateServer 剥离；经 account-core 写账号数据，对外提供 /get_varifycode、/user_register、/reset_pwd。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`app/`](app/_TREE.md) | RegisterServer 进程入口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | RegisterService 构建脚本 |
| `register.ini` | RegisterServer 运行配置 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
