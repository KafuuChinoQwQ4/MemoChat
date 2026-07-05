/**
 * chatDb — Dexie IndexedDB database for message persistence.
 * Mirrors SQLite PrivateChatCacheStore / GroupChatCacheStore.
 * Namespaced per user uid to isolate sessions.
 */
import Dexie, {} from "dexie";
const PRUNE_TO_N = 200; // keep last N messages per conversation
class ChatDatabase extends Dexie {
    constructor() {
        super("MemoChat_v1");
        this.version(1).stores({
            messages: "_key, _ownerUid, _peerId, timestamp",
        });
    }
}
export const chatDb = new ChatDatabase();
/** Hydrate messages for a conversation from IndexedDB */
export async function hydrateMessages(ownerUid, peerId) {
    const rows = await chatDb.messages
        .where(["_ownerUid", "_peerId"])
        .equals([ownerUid, peerId])
        .sortBy("timestamp");
    return rows.map((r) => {
        // eslint-disable-next-line @typescript-eslint/no-unused-vars
        const { _key: _k, _peerId: _p, _ownerUid: _o, ...msg } = r;
        return msg;
    });
}
/** Write-through: persist a message and prune oldest beyond PRUNE_TO_N */
export async function persistMessage(ownerUid, peerId, msg) {
    const key = `${ownerUid}_${peerId}_${msg.clientMsgId}`;
    await chatDb.messages.put({ ...msg, _key: key, _peerId: peerId, _ownerUid: ownerUid });
    // Prune oldest messages if over limit
    const count = await chatDb.messages
        .where(["_ownerUid", "_peerId"])
        .equals([ownerUid, peerId])
        .count();
    if (count > PRUNE_TO_N) {
        const oldest = await chatDb.messages
            .where(["_ownerUid", "_peerId"])
            .equals([ownerUid, peerId])
            .sortBy("timestamp");
        const toDelete = oldest.slice(0, count - PRUNE_TO_N).map((r) => r._key);
        await chatDb.messages.bulkDelete(toDelete);
    }
}
/** Clear all messages for a user session */
export async function clearUserMessages(ownerUid) {
    await chatDb.messages.where("_ownerUid").equals(ownerUid).delete();
}
