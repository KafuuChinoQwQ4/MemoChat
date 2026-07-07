import { afterEach, describe, expect, it, vi } from "vitest";
import { SseStreamClient } from "./SseStreamClient";
function sseResponse(payload) {
    const encoder = new TextEncoder();
    const stream = new ReadableStream({
        start(controller) {
            controller.enqueue(encoder.encode(`data: ${payload}\n\n`));
            controller.close();
        },
    });
    return new Response(stream, { status: 200, headers: { "Content-Type": "text/event-stream" } });
}
describe("SseStreamClient", () => {
    afterEach(() => {
        vi.unstubAllGlobals();
    });
    it("sends bearer auth and serializes the AI stream body", async () => {
        const fetchMock = vi.fn().mockResolvedValue(sseResponse('{"chunk":"ok","is_final":true}'));
        vi.stubGlobal("fetch", fetchMock);
        const chunks = [];
        await new SseStreamClient().start("/ai/chat/stream", { content: "hello", stream: true }, {
            token: "tok",
            onChunk: (chunk) => chunks.push(chunk.chunk),
        });
        expect(fetchMock).toHaveBeenCalledTimes(1);
        const [url, init] = fetchMock.mock.calls[0];
        expect(url).toBe("/ai/chat/stream");
        expect(init.headers).toMatchObject({
            "Content-Type": "application/json",
            Authorization: "Bearer tok",
        });
        expect(typeof init.body).toBe("string");
        expect(JSON.parse(init.body)).toEqual({ content: "hello", stream: true });
        expect(chunks).toEqual(["ok"]);
    });
});
