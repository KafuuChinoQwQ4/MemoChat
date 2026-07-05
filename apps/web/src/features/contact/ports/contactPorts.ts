/** ContactPorts — injected dependencies for the contact feature */
import type { Friend, ApplyEntry } from "@/core/entities/entityTypes"

export interface ContactPorts {
  getFriends(): Friend[]
  getApplyList(): ApplyEntry[]
}

let _ports: ContactPorts | null = null
export function setContactPorts(p: ContactPorts) { _ports = p }
export function getContactPorts(): ContactPorts {
  if (!_ports) throw new Error("ContactPorts not initialized")
  return _ports
}
