# db/ 目录树

> 数据库（主要是 PostgreSQL）的初始化、修复与巡检 SQL/脚本集合。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `fix_postgresql_sequences.sql` | 修复 PostgreSQL 自增序列错位的 SQL |
| `init_postgresql_schema.ps1` | 初始化 PostgreSQL schema 的脚本 |
| `pg_check.sql` | PostgreSQL 健康/一致性检查 SQL |
| `pg_conns.sql` | 查询当前连接情况的 SQL |
| `pg_dashboard.sql` | 综合监控仪表盘查询 SQL |
| `pg_detail.sql` | 库表明细查询 SQL |
| `pg_memo_users.sql` | memo 用户相关数据查询 SQL |
| `pg_settings.sql` | 数据库配置参数查询 SQL |
| `pg_tables.sql` | 表清单/统计查询 SQL |
| `pg_user_table.sql` | 用户表结构/数据查询 SQL |
| `pg_users.sql` | 用户数据查询 SQL |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
