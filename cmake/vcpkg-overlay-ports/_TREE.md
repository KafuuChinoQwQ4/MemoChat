# vcpkg-overlay-ports/ 目录树

> MemoChat 发行构建使用的 vcpkg overlay ports，用于约束第三方产物中的宿主机路径与发行行为。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`libpq/`](libpq/_TREE.md) | 固定 libpq 16.9 的 Release 系统配置目录并保留 baseline 构建定义 |
| [`mongo-c-driver/`](mongo-c-driver/_TREE.md) | 固定 mongo-c-driver 1.30.6 并禁止握手元数据暴露发行构建参数 |
| [`msquic/`](msquic/_TREE.md) | 固定 msquic 内置 OpenSSL 模块查找路径的发行 port |
| [`openssl/`](openssl/_TREE.md) | 固定 OpenSSL 3.6.0 的 Release 运行时目录并清理构建元数据中的开发路径 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `.gitattributes` | 保留 baseline port 与 unified patch 中语义所需的既有行尾格式 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
