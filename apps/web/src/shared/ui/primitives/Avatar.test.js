import { jsx as _jsx } from "react/jsx-runtime";
import { render, screen } from "@testing-library/react";
import { beforeEach, describe, expect, it } from "vitest";
import { useSessionStore } from "@/core/session/sessionStore";
import { Avatar } from "./Avatar";
describe("Avatar", () => {
    beforeEach(() => {
        useSessionStore.getState().clearSession();
        useSessionStore.getState().setLogin({
            uid: 42,
            token: "tok value",
            loginTicket: "ticket",
            ticketExpireMs: Date.now() + 60000,
            refreshToken: "refresh",
            protocolVersion: 3,
            chatEndpoints: [],
            profile: { uid: 42, name: "tester", email: "t@example.test", icon: "" },
        });
    });
    it("does not expose media avatar credentials in the image URL", () => {
        render(_jsx(Avatar, { src: "media/download?asset=avatar-key", name: "Alice" }));
        expect(screen.getByRole("img").getAttribute("src")).toMatch(/^data:image\/svg\+xml,/);
    });
    it("maps built-in head avatar refs to the local fallback image", () => {
        render(_jsx(Avatar, { src: "head_1", name: "Alice" }));
        expect(screen.getByRole("img").getAttribute("src")).toMatch(/^data:image\/svg\+xml,/);
    });
    it("uses the default avatar image when the profile icon is empty", () => {
        render(_jsx(Avatar, { src: "", name: "Alice" }));
        expect(screen.getByRole("img").getAttribute("src")).toMatch(/^data:image\/svg\+xml,/);
        expect(screen.queryByText("AL")).not.toBeInTheDocument();
    });
});
