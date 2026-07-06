import { describe, expect, it } from "vitest"
import { providerAvatarDataUrl, providerDisplayName, providerKey } from "./agentProviderRuntime"

describe("agentProviderRuntime", () => {
  it("matches the desktop provider alias rules for common models", () => {
    expect(providerKey("gpt-4o", "openai")).toBe("openai")
    expect(providerKey("claude-3-5-sonnet", "anthropic")).toBe("claude")
    expect(providerKey("qwen2.5:7b", "ollama")).toBe("qwen")
    expect(providerKey("moonshot-v1", "api-kimi")).toBe("kimi")
  })

  it("returns provider-specific avatar data urls", () => {
    const avatar = providerAvatarDataUrl("deepseek-chat", "api-deepseek")
    const svg = decodeURIComponent(avatar.replace(/^data:image\/svg\+xml,/, ""))

    expect(avatar).toMatch(/^data:image\/svg\+xml,/)
    expect(svg).toContain("DeepSeek")
    expect(svg).toContain(">D<")
    expect(providerDisplayName("unknown-model", "api-custom")).toBe("OpenAI-compatible API")
  })
})
