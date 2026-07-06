import { render, screen } from "@testing-library/react"
import { beforeEach, describe, expect, it } from "vitest"
import { useEntityStore } from "@/core/entities/entityStore"
import { GroupShellContent } from "./GroupShellContent"

describe("GroupShellContent", () => {
  beforeEach(() => {
    useEntityStore.getState().reset()
  })

  it("auto-selects the first group and renders its detail pane", () => {
    useEntityStore.getState().setGroups([
      {
        groupId: 83,
        groupCode: "g123456789",
        name: "Runtime Smoke Full Chat",
        icon: "",
        memberCount: 2,
        announcement: "ship it",
        role: "owner",
      },
    ])

    render(<GroupShellContent />)

    expect(screen.getByRole("button", { name: /Runtime Smoke Full Chat/ })).toBeInTheDocument()
    expect(screen.getByRole("button", { name: "建群" })).toBeInTheDocument()
    expect(screen.getAllByText("Runtime Smoke Full Chat").length).toBeGreaterThan(0)
    expect(screen.getAllByText("2 人").length).toBeGreaterThanOrEqual(2)
    expect(screen.getByText("群号: g123456789")).toBeInTheDocument()
    expect(screen.queryByText("群号: 83")).not.toBeInTheDocument()
    expect(screen.getByText("ship it")).toBeInTheDocument()
    expect(screen.queryByText("选择群组查看详情")).not.toBeInTheDocument()
  })
})
