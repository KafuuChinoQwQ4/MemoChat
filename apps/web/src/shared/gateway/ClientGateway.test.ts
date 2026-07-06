import { createElement, StrictMode } from "react"
import { MemoryRouter } from "react-router-dom"
import { cleanup, render } from "@testing-library/react"
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest"
import { ClientGateway } from "./ClientGateway"
import { ReqId } from "@/core/network/opcodes/reqIds"
import { GatewayProvider } from "@/app/providers/GatewayProvider"
import { AppShell } from "@/routes/shell/AppShell"
import { registerChatRoutes } from "@/app/dispatch/registerChatRoutes"
import { startConnectionCoordinator } from "@/app/bootstrap/connectionCoordinator"

vi.mock("@/app/dispatch/registerChatRoutes", () => ({
  registerChatRoutes: vi.fn(),
}))

vi.mock("@/app/bootstrap/connectionCoordinator", () => ({
  startConnectionCoordinator: vi.fn(),
}))

const unregisterRoutes = vi.fn()
const stopCoordinator = vi.fn()

describe("ClientGateway", () => {
  beforeEach(() => {
    vi.mocked(registerChatRoutes).mockReset()
    vi.mocked(registerChatRoutes).mockReturnValue(unregisterRoutes)
    vi.mocked(startConnectionCoordinator).mockReset()
    vi.mocked(startConnectionCoordinator).mockReturnValue(stopCoordinator)
    unregisterRoutes.mockReset()
    stopCoordinator.mockReset()
  })

  afterEach(() => {
    cleanup()
  })

  it("routes chat transport frames into the dispatcher", () => {
    const gateway = new ClientGateway()
    const frames: Array<{ reqId: ReqId; payload: string }> = []

    gateway.dispatcher.subscribe(ReqId.ID_GET_RELATION_BOOTSTRAP_RSP, (frame) => {
      frames.push(frame)
    })

    gateway.chatTransport.onMessage?.(
      ReqId.ID_GET_RELATION_BOOTSTRAP_RSP,
      JSON.stringify({ error: 0, friend_list: [{ uid: 910002 }] }),
    )

    expect(frames).toEqual([
      {
        reqId: ReqId.ID_GET_RELATION_BOOTSTRAP_RSP,
        length: JSON.stringify({ error: 0, friend_list: [{ uid: 910002 }] }).length,
        payload: JSON.stringify({ error: 0, friend_list: [{ uid: 910002 }] }),
      },
    ])

    gateway.chatTransport.close()
  })

  it("keeps chat route handlers registered after React StrictMode remounts AppShell effects", () => {
    render(
      createElement(
        StrictMode,
        null,
        createElement(
          GatewayProvider,
          null,
          createElement(
            MemoryRouter,
            null,
            createElement(AppShell),
          ),
        ),
      ),
    )

    expect(registerChatRoutes).toHaveBeenCalledTimes(2)
    expect(startConnectionCoordinator).toHaveBeenCalledTimes(2)
    expect(unregisterRoutes).toHaveBeenCalledTimes(1)
    expect(stopCoordinator).toHaveBeenCalledTimes(1)
  })
})
