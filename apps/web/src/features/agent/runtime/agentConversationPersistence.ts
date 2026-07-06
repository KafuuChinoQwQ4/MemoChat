export interface AgentChatMessage {
  role: "user" | "assistant"
  content: string
  modelType?: string
  modelName?: string
}

export interface AgentConversation {
  id: string
  title: string
  updatedAt: number
  messages: AgentChatMessage[]
}

export interface AgentConversationState {
  conversations: AgentConversation[]
  selectedConversationId: string
}

const STORAGE_PREFIX = "memochat:web:agent:conversations:v1"
export const MAX_AGENT_CONVERSATIONS = 30
export const MAX_AGENT_MESSAGES_PER_CONVERSATION = 120
const MAX_MESSAGE_CONTENT_LENGTH = 24_000
const MAX_TITLE_LENGTH = 80

export function agentConversationStorageKey(uid: number): string {
  return `${STORAGE_PREFIX}:u${uid}`
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value)
}

function cleanText(value: unknown, maxLength: number): string | null {
  if (typeof value !== "string") return null
  const trimmed = value.trim()
  if (trimmed.length === 0) return null
  return trimmed.length > maxLength ? trimmed.slice(0, maxLength) : trimmed
}

function cleanOptionalText(value: unknown, maxLength: number): string | undefined {
  if (typeof value !== "string") return undefined
  const trimmed = value.trim()
  if (trimmed.length === 0) return undefined
  return trimmed.length > maxLength ? trimmed.slice(0, maxLength) : trimmed
}

function cleanTimestamp(value: unknown): number {
  if (typeof value !== "number" || !Number.isFinite(value) || value <= 0) {
    return Date.now()
  }
  return value
}

function cleanMessage(value: unknown): AgentChatMessage | null {
  if (!isRecord(value)) return null
  if (value.role !== "user" && value.role !== "assistant") return null
  const content = cleanText(value.content, MAX_MESSAGE_CONTENT_LENGTH)
  if (content === null) return null

  const modelType = cleanOptionalText(value.modelType, 120)
  const modelName = cleanOptionalText(value.modelName, 240)

  return {
    role: value.role,
    content,
    ...(modelType !== undefined ? { modelType } : {}),
    ...(modelName !== undefined ? { modelName } : {}),
  }
}

function cleanConversation(value: unknown): AgentConversation | null {
  if (!isRecord(value)) return null
  const id = cleanText(value.id, 120)
  if (id === null) return null
  const title = cleanText(value.title, MAX_TITLE_LENGTH) ?? "新对话"
  const messages = Array.isArray(value.messages)
    ? value.messages.map(cleanMessage).filter((message): message is AgentChatMessage => message !== null)
    : []

  return {
    id,
    title,
    updatedAt: cleanTimestamp(value.updatedAt),
    messages: messages.slice(-MAX_AGENT_MESSAGES_PER_CONVERSATION),
  }
}

export function pruneAgentConversations(conversations: AgentConversation[]): AgentConversation[] {
  return conversations
    .map((conversation) => ({
      ...conversation,
      messages: conversation.messages.slice(-MAX_AGENT_MESSAGES_PER_CONVERSATION),
    }))
    .sort((a, b) => b.updatedAt - a.updatedAt)
    .slice(0, MAX_AGENT_CONVERSATIONS)
}

export function sanitizeAgentConversationState(value: unknown): AgentConversationState | null {
  if (!isRecord(value) || !Array.isArray(value.conversations)) return null

  const conversations = pruneAgentConversations(
    value.conversations
      .map(cleanConversation)
      .filter((conversation): conversation is AgentConversation => conversation !== null),
  )
  if (conversations.length === 0) return null

  const selected =
    typeof value.selectedConversationId === "string" &&
    conversations.some((conversation) => conversation.id === value.selectedConversationId)
      ? value.selectedConversationId
      : conversations[0]?.id

  if (!selected) return null

  return {
    conversations,
    selectedConversationId: selected,
  }
}

function browserStorage(): Storage | null {
  if (typeof window === "undefined") return null
  try {
    return window.localStorage
  } catch {
    return null
  }
}

export function loadAgentConversationState(uid: number): AgentConversationState | null {
  const storage = browserStorage()
  if (!storage) return null
  const raw = storage.getItem(agentConversationStorageKey(uid))
  if (!raw) return null

  try {
    return sanitizeAgentConversationState(JSON.parse(raw))
  } catch {
    return null
  }
}

export function saveAgentConversationState(uid: number, state: AgentConversationState): void {
  const storage = browserStorage()
  if (!storage) return

  const sanitized = sanitizeAgentConversationState(state)
  if (!sanitized) {
    try {
      storage.removeItem(agentConversationStorageKey(uid))
    } catch {
      // Ignore storage failures; chat can still continue in memory.
    }
    return
  }

  try {
    storage.setItem(agentConversationStorageKey(uid), JSON.stringify(sanitized))
  } catch {
    // Ignore quota or privacy-mode failures; chat can still continue in memory.
  }
}
