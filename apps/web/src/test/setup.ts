import "@testing-library/jest-dom"
import { server } from "./msw/server"
import { beforeAll, afterAll, afterEach } from "vitest"

// Start MSW server before all tests
beforeAll(() => server.listen({ onUnhandledRequest: "warn" }))
afterEach(() => server.resetHandlers())
afterAll(() => server.close())
