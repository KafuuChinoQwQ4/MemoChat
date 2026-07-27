# workflows/ 目录树

> GitHub Actions 工作流定义：PR/推送触发的持续集成与持续部署管线。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ci.yml` | Linux 发布 CI：锁定外部 Action、执行安全门禁，并以 archive/bundle 内容绑定的不可变 SHA/版本标签发布 GHCR 镜像和 GitHub Release |
| `cd.yml` | 制品推广 CD：使用锁定的外部 Action，校验 CI digest 清单后为 GHCR 镜像推广 dev/stable 标签 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
