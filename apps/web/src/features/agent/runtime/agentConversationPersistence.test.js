import { beforeEach, describe, expect, it } from "vitest";
import { MAX_AGENT_CONVERSATIONS, MAX_AGENT_MESSAGES_PER_CONVERSATION, agentConversationStorageKey, loadAgentConversationState, sanitizeAgentConversationState, saveAgentConversationState, } from "./agentConversationPersistence";
function installMemoryStorage() {
    const data = new Map();
    const storage = {
        get length() {
            return data.size;
        },
        clear() {
            data.clear();
        },
        getItem(key) {
            return data.get(key) ?? null;
        },
        key(index) {
            return Array.from(data.keys())[index] ?? null;
        },
        removeItem(key) {
            data.delete(key);
        },
        setItem(key, value) {
            data.set(key, value);
        },
    };
    Object.defineProperty(window, "localStorage", {
        value: storage,
        configurable: true,
    });
    Object.defineProperty(globalThis, "localStorage", {
        value: storage,
        configurable: true,
    });
    return storage;
}
function conversation(id, updatedAt, messageCount = 1) {
    return {
        id,
        title: `会话 ${id}`,
        updatedAt,
        messages: Array.from({ length: messageCount }, (_, index) => ({
            role: index % 2 === 0 ? "user" : "assistant",
            content: `消息 ${index}`,
            modelType: "api-kimi",
            modelName: "moonshot-v1",
        })),
    };
}
describe("agentConversationPersistence", () => {
    beforeEach(() => {
        installMemoryStorage().clear();
    });
    it("round-trips conversations with a per-user storage key", () => {
        const state = {
            conversations: [conversation("local-a", 200), conversation("local-b", 100)],
            selectedConversationId: "local-b",
        };
        saveAgentConversationState(1001, state);
        expect(localStorage.getItem(agentConversationStorageKey(1002))).toBeNull();
        expect(loadAgentConversationState(1001)).toEqual(state);
    });
    it("ignores malformed persisted storage", () => {
        localStorage.setItem(agentConversationStorageKey(1001), "{bad json");
        expect(loadAgentConversationState(1001)).toBeNull();
    });
    it("falls back to the newest available conversation when selected id is stale", () => {
        const sanitized = sanitizeAgentConversationState({
            conversations: [conversation("old", 100), conversation("new", 300)],
            selectedConversationId: "missing",
        });
        expect(sanitized?.selectedConversationId).toBe("new");
    });
    it("drops invalid messages and prunes old history", () => {
        const conversations = Array.from({ length: MAX_AGENT_CONVERSATIONS + 4 }, (_, index) => conversation(`c-${index}`, index + 1, MAX_AGENT_MESSAGES_PER_CONVERSATION + 5));
        const sanitized = sanitizeAgentConversationState({
            conversations: [
                ...conversations,
                { id: "", title: "invalid", updatedAt: 1, messages: [] },
                { id: "bad-message", title: "bad", updatedAt: 500, messages: [{ role: "system", content: "x" }] },
            ],
            selectedConversationId: "c-0",
        });
        expect(sanitized?.conversations).toHaveLength(MAX_AGENT_CONVERSATIONS);
        expect(sanitized?.conversations[0]?.id).toBe("bad-message");
        expect(sanitized?.conversations[1]?.id).toBe(`c-${MAX_AGENT_CONVERSATIONS + 3}`);
        expect(sanitized?.conversations[1]?.messages).toHaveLength(MAX_AGENT_MESSAGES_PER_CONVERSATION);
    });
});
