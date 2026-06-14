# GateServer Transport Adapter Contract

Native transport request -> `GateRequest` -> `RouteRegistry`/shared service -> `GateResponse` -> native transport response.

This document is the repo-local contract for GateServer transport adapters. It freezes the H1/H2/H3 adapter rules for G2, G3, and G4 without changing C++ behavior in G1.

## Ownership Rule

Transport owns protocol mechanics, modules/services own business behavior.

Adapters own native protocol extraction, normalization, trace propagation, dispatch to the shared route model, and native response writing. They do not own account, profile, media, moments, call, AI, R18, verification, persistence, cache, or object-storage behavior once a route has a shared module/service path.

Once a route has a shared module/service, its adapter must not directly call these business dependencies:

- `PostgresMgr`
- `MongoMgr`
- `RedisMgr`
- `VerifyGrpcClient`
- `CallService`
- `Http2MediaSupport`
- `Http2ProfileSupport`
- `R18SourceService`
- `AIServiceClient`

Those dependencies belong behind shared route modules or services. A temporary legacy handler may still call them only while the route is explicitly listed as a legacy adapter exception or has not been migrated into the shared model.

## GateRequest Mapping

Every adapter maps the native request into `memochat::gate::routing::GateRequest` with the same semantics:

- `method`: preserve the native HTTP method as the route key, using the same uppercase method names used by route registration.
- `path`: preserve the normalized path without the query string. Do not include transport framing data.
- `target`: preserve the full native target/URI when available. If a transport exposes only path and query separately, compose the same route target shape used by H1.
- `query`: parse query parameters into key/value strings. Preserve empty values as empty strings. Do not parse product JSON fields into query.
- `headers`: copy request headers that the native transport exposes. Header names are not business fields; adapter code must not interpret them beyond transport, tracing, auth-pass-through, or legacy compatibility needs.
- `body`: copy the request body byte-for-byte into `std::string`, including upload chunks or other binary-compatible payloads. Do not reserialize JSON in the adapter.
- `trace_id`: preserve the native trace id when present; otherwise use the same fallback/generation policy as the transport currently uses. Set `memolog::TraceContext` before shared dispatch.
- `request_id`: preserve the native request id when present; otherwise leave empty or use the transport's existing generated value. Do not invent a business-level id.
- `remote_address`: preserve the peer address when the native transport exposes it; otherwise leave empty or use the current transport fallback such as `unknown`.

Adapters dispatch either through `RouteRegistry::Dispatch(request, response)` or through a shared service facade during route-by-route migration. They must not register duplicate exact business handlers ahead of `RouteRegistry` for routes that have already moved to the shared model.

## GateResponse Mapping

Every adapter maps `memochat::gate::routing::GateResponse` back to the native response with these semantics:

- `status`: write the native HTTP status code without changing the business envelope.
- `content_type`: write the native content type when non-empty; otherwise keep the native/default transport behavior.
- `headers`: copy response headers when the native transport can send them. If it cannot, the route must remain on a documented legacy path or the later slice must add native header support before migration.
- `body_kind`: select the response body strategy. `Inline` writes `body`; `File` writes `file_path` only on transports with a native file response path.
- `body`: write inline response bytes exactly as produced by the shared route/service.
- `file_path`: pass through the file path for native file responses. Do not read file contents in the adapter unless a later slice explicitly defines a safe inline conversion for that endpoint.

Adapters must not alter JSON status fields, response envelopes, auth/cache keys, file identity, or endpoint-specific payload semantics.

## H1 Adapter

Current H1 state in `LogicSystem`:

- `LogicSystem::BuildGateRequest(method, path, HttpConnection)` already builds `GateRequest` from H1 `HttpConnection`, including method, path, target, query, headers, body, trace id, request id, and remote address.
- `LogicSystem::ApplyGateResponse(GateResponse, HttpConnection)` already maps shared responses back to H1, including status, content type, headers, inline body, and file body support.
- Exact legacy handler maps for GET/POST/DELETE run before prefix handlers.
- Legacy prefix handlers run before the shared fallback.
- The shared fallback calls `_route_registry.Dispatch(request, response)` and then `ApplyGateResponse(response, con)`.

G2 must extract those H1 mechanics into a named H1 adapter unit while preserving behavior:

- `BuildGateRequest` behavior moves out of `LogicSystem` without changing the produced `GateRequest`.
- `ApplyGateResponse` behavior moves out of `LogicSystem` without changing the produced H1 response.
- Legacy exact and prefix precedence remains only for intentionally unmigrated routes.
- `LogicSystem` remains route composition/orchestration, not the owner of concrete H1 response writing.

H1 response mapping:

- `GateResponse.status` -> H1 status.
- `GateResponse.content_type` -> H1 `Content-Type` when non-empty.
- `GateResponse.headers` -> H1 response headers.
- `GateResponseBodyKind::Inline` -> H1 inline body.
- `GateResponseBodyKind::File` -> `HttpConnection::SetFileResponse(file_path, content_type)`.

## H2 Adapter

Current H2 state:

- `Http2Routes::RegisterHandler(method, path, handler)` owns the H2 route table and `Http2Routes::HandleRequest` dispatches to registered handlers.
- `Http2Request` currently exposes `method`, `path`, `query`, `body`, `remote_addr`, `trace_id`, and `headers`.
- `Http2Response` currently exposes `status_code`, `status_message`, `body`, `content_type`, `headers`, `SetJsonBody`, `SetStatus`, and `SetHeader`.
- H2 route handlers still call transport-local auth/profile/call/media support in several places.
- `POST /user_logout` exists as a H2-only route today.

G3 conversion rules:

- Add a H2 adapter that converts `Http2Request` to `GateRequest`.
- Parse `Http2Request::query` into `GateRequest::query`.
- Map `remote_addr` to `remote_address`.
- Preserve `trace_id`; add request id only if H2 exposes one in a later slice.
- Keep `RegisterHandler` as the native route registration surface while migrated handlers delegate to the H2 adapter or shared service.
- Prefer `RouteRegistry` dispatch whenever the route already has a shared H1 module/service contract.
- Do not keep a duplicate H2 business handler for a migrated shared route.
- Keep H2-only `/user_logout` documented as H2-only until it is either migrated into a shared module/service or intentionally retired.

H2 response mapping:

- `GateResponse.status` -> `Http2Response::SetStatus(status, status_message)`.
- `GateResponse.content_type` -> `Http2Response::content_type` when non-empty.
- `GateResponse.headers` -> `Http2Response::headers`.
- `GateResponseBodyKind::Inline` -> `Http2Response::body`.
- `GateResponseBodyKind::File` has no current direct H2 file primitive. G3 must add native file support, define an endpoint-specific inline/redirect contract, or leave that endpoint on a documented legacy adapter path.

## H3 Adapter

Current H3 state:

- H3 request handling uses `GateHttp3Connection`.
- `GateHttp3Connection` currently exposes request path, method, body, trace id, request id, request headers, query string, query params, and response inspection fields.
- `GateHttp3Connection::SendResponse(status, body, content_type)` is the current native response path.
- Auth routes already follow the shared model through `BuildHttp3AuthRequest` and `HandleHttp3AuthRoute`.
- Most non-auth H3 routes in `GateHttp3ServiceRoutes.cpp` still contain transport-local JSON parsing or direct business/service calls.

G4 conversion rules:

- Add a H3 adapter that converts `GateHttp3Connection` to `GateRequest`.
- Map `GetRequestMethod()` to `method`.
- Map `GetRequestPath()` to `path`.
- Preserve `GetQueryString()` as the source for `query`; use current parsed query params where available.
- Map `GetRequestHeaders()` to `headers`.
- Map `GetRequestBody()` to `body`.
- Map `GetTraceId()` to `trace_id` and `GetRequestId()` to `request_id`.
- Set `remote_address` only when H3 exposes a peer address; do not fabricate one.
- Reuse `RouteRegistry` or shared services for migrated routes.
- Remove duplicated H3 business behavior once the route has a shared module/service path.

H3 response mapping:

- `GateResponse.status` -> `GateHttp3Connection::SendResponse(status, body, content_type)`.
- `GateResponse.content_type` -> `SendResponse` content type when non-empty, otherwise keep the current default behavior.
- `GateResponse.headers` currently has no native send path in `GateHttp3Connection::SendResponse`; G4 must add response header support or keep header-dependent endpoints on a documented legacy adapter path.
- `GateResponseBodyKind::Inline` -> `SendResponse` body.
- `GateResponseBodyKind::File` has no current direct H3 file primitive. G4 must add native file support, define an endpoint-specific inline/redirect contract, or leave that endpoint on a documented legacy adapter path.

## Legacy Adapter Exceptions

These exceptions are allowed only while explicitly documented and must shrink as shared modules/services become available:

- AI stream and prefix routes may remain as H1 legacy prefix/exact handlers until streaming semantics are represented by a shared service contract.
- File responses may remain H1-only where H2/H3 lack native file response support.
- Header-dependent responses may remain transport-specific where H3 lacks native response header support.
- H2-only `POST /user_logout` remains a H2-only route until a shared module/service owns its behavior.
- Standalone H1.1 behavior in the H2 process is deferred to G5; G1 does not decide whether that path remains, is removed, or is bridged.
- Any unmigrated handler that still calls `PostgresMgr`, `MongoMgr`, `RedisMgr`, `VerifyGrpcClient`, `CallService`, `Http2MediaSupport`, `Http2ProfileSupport`, `R18SourceService`, or `AIServiceClient` must be treated as legacy adapter code unless the dependency is behind the shared module/service boundary.

## Later Slice Gates

G2 acceptance gate:

- H1 adapter mechanics are in a named adapter unit instead of being owned directly by `LogicSystem`.
- Static or unit coverage proves `BuildGateRequest` and `ApplyGateResponse` behavior is preserved.
- H1 exact/prefix legacy precedence and `_route_registry.Dispatch` fallback remain behavior-compatible.
- No product C++ behavior changes beyond the adapter extraction.

G3 acceptance gate:

- H2 has a named adapter from `Http2Request`/`Http2Response` to the shared route model.
- At least the selected migrated H2 route group dispatches through `RouteRegistry` or the shared service boundary.
- Structure or behavior tests prove H2 endpoint registration is not lost.
- `GateResponseBodyKind::File` handling is either supported, explicitly converted for the selected endpoint, or documented as a legacy limitation.
- H2-only `/user_logout` is explicitly handled by shared migration or documented exception.

G4 acceptance gate:

- H3 has a named adapter from `GateHttp3Connection` to the shared route model.
- H3 auth keeps the existing `BuildHttp3AuthRequest`/`HandleHttp3AuthRoute` pattern or is folded into the new adapter without behavior drift.
- At least the selected migrated H3 route group dispatches through `RouteRegistry` or the shared service boundary.
- Duplicated business tokens shrink from `GateHttp3ServiceRoutes.cpp` for migrated routes.
- H3 response header and file limitations are either solved or documented as legacy exceptions before moving affected routes.

G5 acceptance gate:

- Standalone H1.1 behavior in the H2 process has an explicit decision: keep with a documented adapter, bridge to the shared H1/H2 model, or remove if unsupported.
- The decision includes endpoint coverage, compatibility impact, and verification evidence.
- No transport regains ownership of shared business behavior.
