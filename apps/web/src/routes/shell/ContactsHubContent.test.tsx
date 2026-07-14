import { render, screen } from "@testing-library/react"
import userEvent from "@testing-library/user-event"
import { describe, expect, it, vi } from "vitest"
import { ContactsHubContent } from "./ContactsHubContent"

vi.mock("@/features/contact/components/ContactShellContent", () => ({
  ContactShellContent: () => <div>好友视图内容</div>,
}))

vi.mock("@/features/group/components/GroupShellContent", () => ({
  GroupShellContent: () => <div>群聊视图内容</div>,
}))

describe("ContactsHubContent", () => {
  it("switches between friends and group chats inside the contacts page", async () => {
    const user = userEvent.setup()
    render(<ContactsHubContent />)

    expect(screen.getByText("联系人")).toBeInTheDocument()
    expect(screen.getByRole("tab", { name: "好友" })).toHaveAttribute("aria-selected", "true")
    expect(screen.getByText("好友视图内容")).toBeInTheDocument()

    await user.click(screen.getByRole("tab", { name: "群聊" }))

    expect(screen.getByRole("tab", { name: "群聊" })).toHaveAttribute("aria-selected", "true")
    expect(screen.getByText("群聊视图内容")).toBeInTheDocument()
  })
})
