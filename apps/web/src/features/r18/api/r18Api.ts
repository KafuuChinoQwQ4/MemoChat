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

interface R18Page {
  index?: number
  image_id?: string
  url?: string
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

    async search(sourceId: string, keyword: string, page: number) {
      const response = await http.post<R18Envelope<{
        source_id?: string
        items?: R18ComicItem[]
        max_page?: number
        error_message?: string
      }>>(ENDPOINTS.r18Search, {
        source_id: sourceId,
        keyword,
        page,
      })
      return unwrap(response, "R18 search")
    },

    async listChapters(sourceId: string, comicId: string): Promise<R18Chapter[]> {
      const response = await http.post<R18Envelope<{ chapters?: R18Chapter[] }>>(ENDPOINTS.r18ComicDetail, {
        source_id: sourceId,
        comic_id: comicId,
      })
      return unwrap(response, "R18 comic detail").chapters ?? []
    },

    async listPageUrls(sourceId: string, chapterId: string): Promise<string[]> {
      const response = await http.post<R18Envelope<{ pages?: R18Page[] }>>(ENDPOINTS.r18ChapterPages, {
        source_id: sourceId,
        chapter_id: chapterId,
      })
      return (unwrap(response, "R18 chapter pages").pages ?? [])
        .map((page) => page.url?.trim() ?? "")
        .filter(Boolean)
    },
  }
}

export type R18Api = ReturnType<typeof createR18Api>
