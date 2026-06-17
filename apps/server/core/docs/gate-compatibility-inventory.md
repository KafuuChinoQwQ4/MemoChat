# Gate 兼容层(legacy transport)清单

本文档登记 GateShared 下保留的 **legacy transport 兼容代码**边界。这些目录是
GateServer monolith 退役、各业务域剥离为聚焦微服务后保留的协议兼容层,**仅作兼容用途**,
不承载新业务。

## 范围

以下目录是 legacy transport 兼容代码,按协议划分:

- `GateShared/transports/h1/legacy_standalone/**` —— HTTP/1.1 独立网关(opt-in,默认不编译,
  由 `MEMOCHAT_BUILD_LEGACY_GATE_HTTP1_STANDALONE` 控制)。含 H1 监听器、连接、worker 池,
  以及 Auth/Media/Moments/R18 的 H1 路由实现。
- `GateShared/transports/h2/**` —— HTTP/2 独立 handler 与 routes(`handlers/`、`routes/`),
  保留 Auth/Call/Media/Profile 的 H2 兼容处理。
- `GateShared/transports/h3/legacy_routes/**` —— HTTP/3 legacy 路由
  (`GateHttp3ServiceRoutes`)。

## 约定

**Do not add new product routes here**(不要在此处新增业务路由)。新业务路由应放在
GateShared 的 **focused route modules**(`GateShared/modules/**`)及各聚合微服务
(AccountShared / MediaService / MomentsService / CallService / R18Service / AIGatewayService)
的 domain 路由模块中。

这些 legacy transport 目录的文件集合受 `test_gate_compatibility_inventory.py::
test_legacy_transport_route_directories_stay_inventory_only` 锁定:改动它们(新增/删除文件)
会让该契约测试失败,提示把新路由放到 focused route modules,仅在有意做兼容工作时才更新本清单。
