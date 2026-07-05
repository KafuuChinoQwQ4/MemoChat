import type { ContactPorts } from "@/features/contact/ports/contactPorts"
import { useEntityStore } from "@/core/entities/entityStore"
import type { DIContainer } from "@/app/container/DIContainer"

export function bindContactPorts(_container: DIContainer): ContactPorts {
  return {
    getFriends: () => Array.from(useEntityStore.getState().friends.values()),
    getApplyList: () => useEntityStore.getState().applyList,
  }
}
