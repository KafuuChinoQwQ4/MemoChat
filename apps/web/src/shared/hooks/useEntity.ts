import { useEntityStore } from "@/core/entities/entityStore"

export function useFriend(uid: number) {
  return useEntityStore((s) => s.friends.get(uid))
}

export function useDialogList() {
  return useEntityStore((s) => s.getDialogList())
}

export function useMessages(peerId: number) {
  return useEntityStore((s) => s.getMessages(peerId))
}
