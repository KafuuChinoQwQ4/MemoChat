import { http, HttpResponse } from "msw";
/** MSW request handlers — one entry per API route */
export const handlers = [
    // Auth routes
    http.post("/user_login", () => HttpResponse.json({
        error: 0,
        user: {
            uid: 1001,
            name: "TestUser",
            email: "test@example.com",
            icon: "",
            access_token: "mock-access-token",
            login_ticket: "mock-ticket",
            ticket_expire_ms: Date.now() + 7 * 86400 * 1000,
            refresh_token: "mock-refresh",
            protocol_version: 3,
            chat_endpoints: [{ host: "localhost", port: "9000", priority: 1 }],
        },
    })),
    http.post("/get_varifycode", () => HttpResponse.json({ error: 0 })),
    http.post("/user_register", () => HttpResponse.json({ error: 0, uid: 1002 })),
    http.post("/reset_pwd", () => HttpResponse.json({ error: 0 })),
    http.get("/get_user_info", () => HttpResponse.json({
        error: 0,
        user: { uid: 1001, name: "TestUser", email: "test@example.com", icon: "" },
    })),
];
