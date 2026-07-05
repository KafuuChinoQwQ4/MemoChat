/**
 * DIContainer — mirrors AppComposition.
 * Holds assembled feature {store, api} pairs and the gateway.
 * Never imported directly by features — they get their deps through ports.
 */
import type { ClientGateway } from "@/shared/gateway/ClientGateway"

export interface DIContainer {
  gateway: ClientGateway
}

let _container: DIContainer | null = null

export function buildContainer(gateway: ClientGateway): DIContainer {
  _container = { gateway }
  return _container
}

export function getContainer(): DIContainer {
  if (!_container) throw new Error("DI container not built yet — call buildContainer first")
  return _container
}

export function teardownContainer(): void {
  _container = null
}
