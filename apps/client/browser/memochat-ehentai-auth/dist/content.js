// Content script bridge for MemoChat web page
// Accepts start messages from the MemoChat origin and forwards to background service worker
const EXTENSION_MESSAGE_VERSION = 1;
// Listen for messages from the MemoChat web page
window.addEventListener('message', async (event) => {
    // Only accept messages from the page itself
    if (event.source !== window)
        return;
    const message = event.data;
    if (!message || message.source !== 'memochat-web')
        return;
    // Validate origin (only localhost/127.0.0.1)
    const origin = event.origin;
    try {
        const url = new URL(origin);
        if (!['localhost', '127.0.0.1'].includes(url.hostname)) {
            console.warn('[MemoChat EH Auth] Rejected message from invalid origin:', origin);
            return;
        }
    }
    catch {
        return;
    }
    if (message.type === 'EHENTAI_BRIDGE_CHECK') {
        // Respond to bridge detection
        window.postMessage({
            source: 'memochat-ehentai-extension',
            type: 'EHENTAI_BRIDGE_READY',
            version: EXTENSION_MESSAGE_VERSION
        }, '*');
        return;
    }
    if (message.type === 'EHENTAI_START_IMPORT') {
        // Forward start import request to background
        try {
            const response = await chrome.runtime.sendMessage({
                type: 'MEMOCHAT_EHENTAI_START_IMPORT',
                payload: {
                    importId: message.importId,
                    ticket: message.ticket,
                    memochatOrigin: origin,
                    expiresAt: message.expiresAt
                }
            });
            window.postMessage({
                source: 'memochat-ehentai-extension',
                type: 'EHENTAI_START_RESPONSE',
                success: response.success,
                error: response.error
            }, '*');
        }
        catch (err) {
            window.postMessage({
                source: 'memochat-ehentai-extension',
                type: 'EHENTAI_START_RESPONSE',
                success: false,
                error: err instanceof Error ? err.message : 'Extension error'
            }, '*');
        }
        return;
    }
    if (message.type === 'EHENTAI_GET_STATUS') {
        // Get current status from background
        try {
            const response = await chrome.runtime.sendMessage({
                type: 'MEMOCHAT_EHENTAI_GET_STATUS'
            });
            window.postMessage({
                source: 'memochat-ehentai-extension',
                type: 'EHENTAI_STATUS_RESPONSE',
                success: response.success,
                data: response.data
            }, '*');
        }
        catch (err) {
            window.postMessage({
                source: 'memochat-ehentai-extension',
                type: 'EHENTAI_STATUS_RESPONSE',
                success: false,
                error: err instanceof Error ? err.message : 'Extension error'
            }, '*');
        }
        return;
    }
    if (message.type === 'EHENTAI_CANCEL') {
        // Cancel current transaction
        try {
            await chrome.runtime.sendMessage({
                type: 'MEMOCHAT_EHENTAI_CANCEL'
            });
        }
        catch (err) {
            console.error('[MemoChat EH Auth] Cancel failed:', err);
        }
        return;
    }
});
// Announce extension presence on load
window.postMessage({
    source: 'memochat-ehentai-extension',
    type: 'EHENTAI_BRIDGE_READY',
    version: EXTENSION_MESSAGE_VERSION
}, '*');
export {};
