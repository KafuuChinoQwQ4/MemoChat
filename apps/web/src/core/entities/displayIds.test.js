import { describe, expect, it } from "vitest";
import { displayNameWithoutInternalId, groupPublicIdText, normalizeGroupPublicId, normalizePublicUserId, publicUserIdText, } from "./displayIds";
describe("display id helpers", () => {
    it("keeps only public user ids for user id labels", () => {
        expect(normalizePublicUserId("u123456789")).toBe("u123456789");
        expect(publicUserIdText("1060")).toBe("未分配用户ID");
        expect(publicUserIdText("u012345678")).toBe("未分配用户ID");
    });
    it("normalizes group public ids to lower-case g-prefixed labels", () => {
        expect(normalizeGroupPublicId("G123456789")).toBe("g123456789");
        expect(groupPublicIdText("g123456789")).toBe("g123456789");
        expect(groupPublicIdText("83")).toBe("未分配群号");
    });
    it("does not use internal ids as display names", () => {
        expect(displayNameWithoutInternalId("1060", "u955561854", 1060, "未知用户")).toBe("u955561854");
        expect(displayNameWithoutInternalId("群号: 83", "g123456789", 83, "未命名群聊")).toBe("g123456789");
        expect(displayNameWithoutInternalId("Yang", "u955561854", 1060, "未知用户")).toBe("Yang");
    });
});
