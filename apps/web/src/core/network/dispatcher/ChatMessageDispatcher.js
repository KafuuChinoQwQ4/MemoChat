export class ChatMessageDispatcher {
    constructor() {
        this._handlers = new Map();
    }
    /**
     * Register a handler for a reqId.
     * Returns an unsubscribe function — always call it on cleanup.
     */
    subscribe(reqId, handler) {
        let set = this._handlers.get(reqId);
        if (!set) {
            set = new Set();
            this._handlers.set(reqId, set);
        }
        set.add(handler);
        return () => {
            this._handlers.get(reqId)?.delete(handler);
        };
    }
    /**
     * Dispatch a decoded frame to all registered handlers.
     * Unknown reqIds are silently dropped (mirrors C++ no-op).
     */
    dispatch(frame) {
        const set = this._handlers.get(frame.reqId);
        if (!set)
            return;
        for (const handler of set) {
            try {
                handler(frame);
            }
            catch (err) {
                console.error("[Dispatcher] handler threw for reqId", frame.reqId, err);
            }
        }
    }
    /** Remove all registered handlers (call on logout/teardown) */
    clear() {
        this._handlers.clear();
    }
}
