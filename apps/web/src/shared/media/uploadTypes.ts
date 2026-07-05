export interface MediaUploadResult {
  url: string
  key: string
  mimeType: string
  sizeBytes: number
}

export interface UploadRequest {
  file: File
  onProgress?: (pct: number) => void
}
