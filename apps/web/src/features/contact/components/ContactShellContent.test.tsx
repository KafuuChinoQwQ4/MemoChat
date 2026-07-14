import { render, screen } from "@testing-library/react"
import { beforeEach, describe, expect, it, vi } from "vitest"
import { useEntityStore } from "@/core/entities/entityStore"
import { ContactShellContent } from "./ContactShellContent"

vi.mock("@/shared/gateway/ClientGateway", () => ({
  getGateway: () => ({
    dispatcher: { subscribe: () => () => undefined },
    chatTransport: { send: vi.fn() },
  }),
}))

describe("ContactShellContent", () => {
  beforeEach(() => {
    useEntityStore.getState().reset()
    useEntityStore.getState().setFriends([
      { uid: 1, name: "张三", email: "", icon: "", userId: "u123456781" },
      { uid: 2, name: "alice", email: "", icon: "", userId: "u123456782" },
      { uid: 3, name: "@系统好友", email: "", icon: "", userId: "u123456783" },
    ])
  })

  it("groups friends by alphabetic initial and puts special names in #", () => {
    render(<ContactShellContent />)

    expect(screen.getByRole("heading", { name: "A", level: 2 })).toBeInTheDocument()
    expect(screen.getByRole("heading", { name: "Z", level: 2 })).toBeInTheDocument()
    expect(screen.getByRole("heading", { name: "#", level: 2 })).toBeInTheDocument()
    expect(screen.getByRole("button", { name: /alice/ })).toBeInTheDocument()
    expect(screen.getByRole("button", { name: /张三/ })).toBeInTheDocument()
    expect(screen.getByRole("button", { name: /系统好友/ })).toBeInTheDocument()
  })
})
