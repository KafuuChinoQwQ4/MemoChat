/**
 * PortRegistry — mirrors AppPortRegistry.
 * Aggregates typed port objects for each feature. Built by bindAllPorts()
 * during post-login bootstrap, before features render.
 */
import type { ChatPorts } from "@/features/chat/ports/chatPorts"
import type { ContactPorts } from "@/features/contact/ports/contactPorts"

export interface PortRegistry {
  chat: ChatPorts
  contact: ContactPorts
}

let _registry: PortRegistry | null = null

export function setPortRegistry(r: PortRegistry) { _registry = r }
export function getPortRegistry(): PortRegistry {
  if (!_registry) throw new Error("PortRegistry not initialized")
  return _registry
}
export function clearPortRegistry() { _registry = null }
