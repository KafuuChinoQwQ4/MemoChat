import { jsx as _jsx } from "react/jsx-runtime";
import { render, screen } from "@testing-library/react";
import { beforeEach, describe, expect, it } from "vitest";
import { useEntityStore } from "@/core/entities/entityStore";
import { GroupShellContent } from "./GroupShellContent";
describe("GroupShellContent", () => {
    beforeEach(() => {
        useEntityStore.getState().reset();
    });
    it("auto-selects the first group and renders its detail pane", () => {
        useEntityStore.getState().setGroups([
            {
                groupId: 83,
                name: "Runtime Smoke Full Chat",
                icon: "",
                memberCount: 2,
                announcement: "ship it",
                role: "owner",
            },
        ]);
        render(_jsx(GroupShellContent, {}));
        expect(screen.getByRole("button", { name: /Runtime Smoke Full Chat/ })).toBeInTheDocument();
        expect(screen.getAllByText("Runtime Smoke Full Chat").length).toBeGreaterThan(0);
        expect(screen.getAllByText("2 人").length).toBeGreaterThanOrEqual(2);
        expect(screen.getByText("ship it")).toBeInTheDocument();
        expect(screen.queryByText("选择群组查看详情")).not.toBeInTheDocument();
    });
});
