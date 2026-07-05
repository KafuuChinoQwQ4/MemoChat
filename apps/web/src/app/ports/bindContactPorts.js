import { useEntityStore } from "@/core/entities/entityStore";
export function bindContactPorts(_container) {
    return {
        getFriends: () => Array.from(useEntityStore.getState().friends.values()),
        getApplyList: () => useEntityStore.getState().applyList,
    };
}
