import { fireEvent, render, screen } from "@testing-library/react"
import { describe, expect, it, vi } from "vitest"
import type { Group } from "@/core/entities/entityTypes"
import { GroupManagementPanel } from "./GroupManagementPanel"

const ownerGroup: Group = {
  groupId: 83,
  groupCode: "g123456789",
  name: "Runtime Smoke Full Chat",
  icon: "",
  memberCount: 3,
  announcement: "ship it",
  role: "owner",
  canChangeInfo: true,
  canDeleteMessages: true,
  canInviteUsers: true,
  canManageAdmins: true,
  canPinMessages: true,
  canBanUsers: true,
  canManageTopics: false,
}

describe("GroupManagementPanel", () => {
  it("renders desktop-aligned management actions for owner", () => {
    const actions = {
      inviteGroupMember: vi.fn(),
      kickGroupMember: vi.fn(),
      setGroupAdmin: vi.fn(),
      muteGroupMember: vi.fn(),
      reviewGroupApply: vi.fn(),
      updateGroupAnnouncement: vi.fn(),
      updateGroupIconFromFile: vi.fn(),
      quitGroup: vi.fn(),
      dissolveGroup: vi.fn(),
      refreshGroupList: vi.fn(),
    }

    render(
      <GroupManagementPanel
        group={ownerGroup}
        friends={[
          {
            uid: 910002,
            name: "Bob",
            email: "b@example.test",
            icon: "",
            userId: "u223456789",
          },
        ]}
        open
        onClose={vi.fn()}
        actions={actions}
      />,
    )

    expect(screen.getByRole("dialog", { name: "群管理" })).toBeInTheDocument()
    expect(screen.getByText("修改群头像")).toBeInTheDocument()
    expect(screen.getByText("解散群聊")).toBeInTheDocument()
    expect(screen.getByText("邀请")).toBeInTheDocument()
    expect(screen.getByText("踢出群聊")).toBeInTheDocument()

    fireEvent.change(screen.getByLabelText("目标用户ID"), { target: { value: "u223456789" } })
    fireEvent.click(screen.getByText("邀请"))
    expect(actions.inviteGroupMember).toHaveBeenCalledWith("u223456789", "")

    fireEvent.click(screen.getByText("踢出群聊"))
    expect(actions.kickGroupMember).toHaveBeenCalledWith("u223456789")
  })

  it("shows quit instead of dissolve for non-owner", () => {
    const actions = {
      inviteGroupMember: vi.fn(),
      kickGroupMember: vi.fn(),
      setGroupAdmin: vi.fn(),
      muteGroupMember: vi.fn(),
      reviewGroupApply: vi.fn(),
      updateGroupAnnouncement: vi.fn(),
      updateGroupIconFromFile: vi.fn(),
      quitGroup: vi.fn(),
      dissolveGroup: vi.fn(),
      refreshGroupList: vi.fn(),
    }

    render(
      <GroupManagementPanel
        group={{ ...ownerGroup, role: "member", canChangeInfo: false, canInviteUsers: false, canManageAdmins: false, canBanUsers: false }}
        friends={[]}
        open
        onClose={vi.fn()}
        actions={actions}
      />,
    )

    expect(screen.getByText("退群")).toBeInTheDocument()
    expect(screen.queryByText("解散群聊")).not.toBeInTheDocument()
    expect(screen.getByText("暂无可用管理权限")).toBeInTheDocument()
  })
})

  it("shows member list and pending join applies", () => {
    const actions = {
      inviteGroupMember: vi.fn(),
      kickGroupMember: vi.fn(),
      setGroupAdmin: vi.fn(),
      muteGroupMember: vi.fn(),
      reviewGroupApply: vi.fn(),
      updateGroupAnnouncement: vi.fn(),
      updateGroupIconFromFile: vi.fn(),
      quitGroup: vi.fn(),
      dissolveGroup: vi.fn(),
      refreshGroupList: vi.fn(),
    }

    render(
      <GroupManagementPanel
        group={ownerGroup}
        friends={[]}
        members={[
          {
            uid: 910001,
            userId: "u123456789",
            name: "Alice",
            role: "owner",
            source: "self",
          },
          {
            uid: 910002,
            userId: "u223456789",
            name: "Bob",
            role: "member",
            source: "message",
          },
        ]}
        pendingApplies={[
          {
            applyId: 42,
            groupId: 83,
            applicantUid: 910003,
            applicantUserId: "u323456789",
            reason: "想加入",
          },
        ]}
        open
        onClose={vi.fn()}
        actions={actions}
      />,
    )

    expect(screen.getByText("群成员")).toBeInTheDocument()
    expect(screen.getByText("Alice")).toBeInTheDocument()
    expect(screen.getByText("Bob")).toBeInTheDocument()
    expect(screen.getByText("入群审核")).toBeInTheDocument()
    expect(screen.getByText("申请单 #42")).toBeInTheDocument()
    const agreeButtons = screen.getAllByText("同意")
    fireEvent.click(agreeButtons[0]!)
    expect(actions.reviewGroupApply).toHaveBeenCalledWith(42, true)
  })
