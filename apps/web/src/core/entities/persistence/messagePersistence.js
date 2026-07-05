/**
 * messagePersistence — bridges entityStore and chatDb.
 * Called by chat feature on new messages and bootstrap.
 */
import { hydrateMessages, persistMessage } from "./chatDb";
import { useEntityStore } from "@/core/entities/entityStore";
/** Load persisted messages for a conversation into the entity store */
export async function hydrateConversation(ownerUid, peerId) {
    const msgs = await hydrateMessages(ownerUid, peerId);
    if (msgs.length > 0) {
        useEntityStore.getState().prependMessages(peerId, msgs);
    }
}
/** Persist a new message and append it to the entity store */
export async function saveAndAppend(ownerUid, peerId, msg) {
    useEntityStore.getState().appendMessage(peerId, msg);
    await persistMessage(ownerUid, peerId, msg);
}
