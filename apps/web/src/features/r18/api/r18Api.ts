import { ENDPOINTS } from "@/core/config/endpoints"
import type { HttpClient } from "@/core/network/http/HttpClient"

interface R18Envelope<T> {
  error: number
  message?: string
  data?: T
}

export interface R18AccessPolicy {
  allowed: boolean
  adult_attested_at_ms: number
  state: "denied" | "allowed" | "revoked"
  can_attest: boolean
}

export interface R18Source {
  id: string
  name?: string
  title?: string
  version?: string
  enabled?: boolean
  builtin?: boolean
  status?: string
  message?: string
  auth_required?: boolean
  direct_access?: boolean
  account_status?: string
  account_username?: string
  has_account?: boolean
}

export interface R18ManagedAccount {
  source_id: string
  name?: string
  auth_required?: boolean
  direct_access?: boolean
  username?: string
  has_password?: boolean
  has_session?: boolean
  status?: string
  message?: string
  updated_at_ms?: number
}

export interface R18AccountsPayload {
  managed?: R18ManagedAccount[]
  accounts?: R18ManagedAccount[]
  sources?: R18Source[]
}

export interface R18ComicItem {
  source_id?: string
  comic_id?: string
  title?: string
  subtitle?: string
  description?: string
  cover?: string
  author?: string
  tags?: string[]
}

export interface R18Chapter {
  chapter_id: string
  title?: string
  index?: number
  order?: number
}

export interface R18VideoSource {
  url: string
  mime_type: "video/mp4"
  quality: number
}

export interface R18VideoPlayback {
  source_id: string
  chapter_id: string
  poster?: string
  expires_at_ms: number
  sources: R18VideoSource[]
}

interface R18Page {
  index?: number
  image_id?: string
  url?: string
}

export interface R18CheckinResult {
  source_id: string
  uid?: string
  status: "ok" | "already" | "not_logged_in" | "unsupported" | "error"
  message?: string
}

export interface R18LibraryFolder {
  id: string
  name: string
  created_at_ms?: number
  updated_at_ms?: number
}

export interface R18LibraryItem {
  source_id: string
  comic_id: string
  title?: string
  cover?: string
  author?: string
  subtitle?: string
  folder_ids?: string[]
  favorited?: boolean
  favorited_at_ms?: number
  updated_at_ms?: number
}

export interface R18BrowserImportStartResponse {
  import_id: string
  ticket: string
  expires_at_ms: number
}

export interface R18BrowserImportStatusResponse {
  status: "pending" | "authenticated" | "failed" | "expired"
  message?: string
}

export interface R18SessionImportInput {
  sourceId: string
  // E-Hentai / ExHentai specific fields (used when sourceId is ehentai/exhentai)
  ipb_member_id?: string
  ipb_pass_hash?: string
  igneous?: string
  sk?: string
  // Generic cookie_header for other sources (nhentai, hanime1, …)
  cookie_header?: string
}

export interface R18SessionImportResponse {
  success: boolean
  message?: string
  ehentai_access: boolean
  exhentai_access: boolean
}

export interface R18LibraryPayload {
  folders?: R18LibraryFolder[]
  items?: R18LibraryItem[]
}

export interface R18FavoriteToggleInput {
  sourceId: string
  comicId: string
  favorited: boolean
  title?: string
  cover?: string
  author?: string
  subtitle?: string
  folderIds?: string[]
}

function unwrap<T>(response: R18Envelope<T>, operation: string): T {
  if (response.error !== 0 || response.data === undefined) {
    throw new Error(response.message || `${operation} failed: ${response.error}`)
  }
  return response.data
}

export function createR18Api(http: HttpClient) {
  return {
    async getAccess(): Promise<R18AccessPolicy> {
      const response = await http.get<R18Envelope<R18AccessPolicy>>(ENDPOINTS.r18Access)
      return unwrap(response, "R18 access lookup")
    },

    async attestAdult(): Promise<R18AccessPolicy> {
      const response = await http.post<R18Envelope<R18AccessPolicy>>(ENDPOINTS.r18AccessAttest, {})
      return unwrap(response, "R18 adult attestation")
    },

    async listSources(): Promise<R18Source[]> {
      const response = await http.get<R18Envelope<{ sources?: R18Source[] }>>(ENDPOINTS.r18Sources)
      return unwrap(response, "R18 sources").sources ?? []
    },

    async search(
      sourceId: string,
      keyword: string,
      page: number,
      options?: { sort?: string; tag?: string },
    ) {
      const response = await http.post<R18Envelope<{
        source_id?: string
        items?: R18ComicItem[]
        max_page?: number
        error_message?: string
        sort?: string
        tag?: string
      }>>(ENDPOINTS.r18Search, {
        source_id: sourceId,
        keyword,
        page,
        sort: options?.sort ?? "",
        tag: options?.tag ?? "",
      })
      return unwrap(response, "R18 search")
    },

    async listChapters(sourceId: string, comicId: string, signal?: AbortSignal): Promise<R18Chapter[]> {
      const body = {
        source_id: sourceId,
        comic_id: comicId,
      }
      const response = signal
        ? await http.post<R18Envelope<{
            title?: string
            description?: string
            error_message?: string
            chapters?: R18Chapter[]
          }>>(ENDPOINTS.r18ComicDetail, body, { signal })
        : await http.post<R18Envelope<{
            title?: string
            description?: string
            error_message?: string
            chapters?: R18Chapter[]
          }>>(ENDPOINTS.r18ComicDetail, body)
      const detail = unwrap(response, "R18 comic detail")
      const errorMessage = detail.error_message?.trim()
        || ((detail.title === "官方源请求失败" || detail.title === "内容源请求失败")
          ? detail.description?.trim()
          : "")
      if (errorMessage) throw new Error(errorMessage)
      return detail.chapters ?? []
    },

    async listPageUrls(sourceId: string, chapterId: string, signal?: AbortSignal): Promise<string[]> {
      const body = {
        source_id: sourceId,
        chapter_id: chapterId,
      }
      const response = signal
        ? await http.post<R18Envelope<{ pages?: R18Page[]; error_message?: string }>>(
            ENDPOINTS.r18ChapterPages,
            body,
            { signal },
          )
        : await http.post<R18Envelope<{ pages?: R18Page[]; error_message?: string }>>(
            ENDPOINTS.r18ChapterPages,
            body,
          )
      const pages = unwrap(response, "R18 chapter pages")
      if (pages.error_message?.trim()) throw new Error(pages.error_message.trim())
      return (pages.pages ?? [])
        .map((page) => page.url?.trim() ?? "")
        .filter(Boolean)
    },

    async resolveVideo(
      sourceId: string,
      chapterId: string,
      signal?: AbortSignal,
    ): Promise<R18VideoPlayback> {
      const body = {
        source_id: sourceId,
        chapter_id: chapterId,
      }
      const response = signal
        ? await http.post<R18Envelope<R18VideoPlayback>>(ENDPOINTS.r18VideoResolve, body, { signal })
        : await http.post<R18Envelope<R18VideoPlayback>>(ENDPOINTS.r18VideoResolve, body)
      return unwrap(response, "R18 video resolve")
    },

    async listAccounts(): Promise<R18AccountsPayload> {
      const response = await http.get<R18Envelope<R18AccountsPayload>>(ENDPOINTS.r18Accounts)
      return unwrap(response, "R18 accounts")
    },

    async saveAccount(sourceId: string, username: string, password: string): Promise<R18AccountsPayload> {
      const response = await http.post<R18Envelope<R18AccountsPayload>>(ENDPOINTS.r18AccountSave, {
        source_id: sourceId,
        username,
        password,
      })
      return unwrap(response, "R18 account save")
    },

    async loginAccount(sourceId: string, username?: string, password?: string): Promise<R18AccountsPayload> {
      const response = await http.post<R18Envelope<R18AccountsPayload>>(ENDPOINTS.r18AccountLogin, {
        source_id: sourceId,
        username: username ?? "",
        password: password ?? "",
      })
      return unwrap(response, "R18 account login")
    },

    async clearAccount(sourceId: string): Promise<R18AccountsPayload> {
      const response = await http.post<R18Envelope<R18AccountsPayload>>(ENDPOINTS.r18AccountClear, {
        source_id: sourceId,
      })
      return unwrap(response, "R18 account clear")
    },

    async startBrowserImport(sourceId: string): Promise<R18BrowserImportStartResponse> {
      const response = await http.post<R18Envelope<R18BrowserImportStartResponse>>(
        ENDPOINTS.r18AccountBrowserImportStart,
        { source_id: sourceId, client_kind: "web_extension" },
      )
      return unwrap(response, "R18 browser import start")
    },

    async getBrowserImportStatus(importId: string): Promise<R18BrowserImportStatusResponse> {
      const response = await http.get<R18Envelope<R18BrowserImportStatusResponse>>(
        `${ENDPOINTS.r18AccountBrowserImportStatus}?import_id=${encodeURIComponent(importId)}`,
      )
      return unwrap(response, "R18 browser import status")
    },

    async importSession(input: R18SessionImportInput): Promise<R18SessionImportResponse> {
      const isEhentai = input.sourceId === "ehentai.official" || input.sourceId === "exhentai.official"
      const response = await http.post<R18Envelope<R18SessionImportResponse>>(
        ENDPOINTS.r18AccountSessionImport,
        isEhentai
          ? {
              source_id: input.sourceId,
              cookies: {
                ipb_member_id: input.ipb_member_id ?? "",
                ipb_pass_hash: input.ipb_pass_hash ?? "",
                igneous: input.igneous ?? "",
                sk: input.sk ?? "",
              },
            }
          : {
              source_id: input.sourceId,
              cookie_header: input.cookie_header ?? "",
            },
      )
      const imported = unwrap(response, "R18 session import")
      if (!imported.success) throw new Error(imported.message?.trim() || "Session import failed")
      return imported
    },

    async checkin(sourceId: string = "jm.official"): Promise<R18CheckinResult> {
      const response = await http.post<R18Envelope<R18CheckinResult>>(ENDPOINTS.r18Checkin, {
        source_id: sourceId,
      })
      return unwrap(response, "R18 check-in")
    },

    async getLibrary(): Promise<R18LibraryPayload> {
      const response = await http.get<R18Envelope<R18LibraryPayload>>(ENDPOINTS.r18Library)
      return unwrap(response, "R18 library")
    },

    async listFavorites(folderId?: string): Promise<{ folder_id?: string; items?: R18LibraryItem[] }> {
      const query = folderId ? `?folder_id=${encodeURIComponent(folderId)}` : ""
      const response = await http.get<R18Envelope<{ folder_id?: string; items?: R18LibraryItem[] }>>(
        `${ENDPOINTS.r18Favorites}${query}`,
      )
      return unwrap(response, "R18 favorites")
    },

    async toggleFavorite(input: R18FavoriteToggleInput): Promise<R18LibraryItem> {
      const response = await http.post<R18Envelope<R18LibraryItem>>(ENDPOINTS.r18FavoriteToggle, {
        source_id: input.sourceId,
        comic_id: input.comicId,
        favorited: input.favorited,
        title: input.title ?? "",
        cover: input.cover ?? "",
        author: input.author ?? "",
        subtitle: input.subtitle ?? "",
        folder_ids: input.folderIds ?? [],
      })
      return unwrap(response, "R18 favorite toggle")
    },

    async assignFavoriteFolders(sourceId: string, comicId: string, folderIds: string[]): Promise<R18LibraryItem> {
      const response = await http.post<R18Envelope<R18LibraryItem>>(ENDPOINTS.r18FavoriteAssign, {
        source_id: sourceId,
        comic_id: comicId,
        folder_ids: folderIds,
      })
      return unwrap(response, "R18 favorite assign")
    },

    async createFolder(name: string): Promise<R18LibraryFolder> {
      const response = await http.post<R18Envelope<R18LibraryFolder>>(ENDPOINTS.r18FolderCreate, { name })
      return unwrap(response, "R18 folder create")
    },

    async renameFolder(folderId: string, name: string): Promise<R18LibraryFolder> {
      const response = await http.post<R18Envelope<R18LibraryFolder>>(ENDPOINTS.r18FolderRename, {
        folder_id: folderId,
        name,
      })
      return unwrap(response, "R18 folder rename")
    },

    async deleteFolder(folderId: string): Promise<R18LibraryPayload> {
      const response = await http.post<R18Envelope<R18LibraryPayload>>(ENDPOINTS.r18FolderDelete, {
        folder_id: folderId,
      })
      return unwrap(response, "R18 folder delete")
    },
  }
}

export type R18Api = ReturnType<typeof createR18Api>
