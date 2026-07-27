import { useCallback, useEffect, useMemo, useRef, useState } from "react"
import type { R18VideoPlayback, R18VideoSource } from "@/features/r18/api/r18Api"
import { useMediaUrl } from "@/shared/hooks/useMediaUrl"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { Spinner } from "@/shared/ui/primitives/Spinner"
import styles from "./R18ShellContent.module.css"

interface R18VideoPlayerOverlayProps {
  sourceId: string
  chapterId: string
  title: string
  resolveVideo: (
    sourceId: string,
    chapterId: string,
    signal: AbortSignal,
  ) => Promise<R18VideoPlayback>
  onClose: () => void
}

interface ResumeState {
  time: number
  shouldResume: boolean
}

const FOCUSABLE_SELECTOR = [
  "button:not([disabled])",
  "a[href]",
  "select:not([disabled])",
  "video[controls]",
  '[tabindex]:not([tabindex="-1"])',
].join(",")

function preferredSource(sources: R18VideoSource[]): R18VideoSource | null {
  return sources.find((source) => source.quality === 720)
    ?? sources.reduce<R18VideoSource | null>((best, source) => (
      best === null || source.quality > best.quality ? source : best
    ), null)
}

function errorMessage(reason: unknown): string {
  if (reason instanceof Error && reason.message.trim()) return reason.message
  return "视频地址解析失败"
}

export function R18VideoPlayerOverlay({
  sourceId,
  chapterId,
  title,
  resolveVideo,
  onClose,
}: R18VideoPlayerOverlayProps) {
  const dialogRef = useRef<HTMLDivElement>(null)
  const videoRef = useRef<HTMLVideoElement>(null)
  const resumeRef = useRef<ResumeState | null>(null)
  const [attempt, setAttempt] = useState(0)
  const [playback, setPlayback] = useState<R18VideoPlayback | null>(null)
  const [selectedQuality, setSelectedQuality] = useState<number | null>(null)
  const [isLoading, setIsLoading] = useState(true)
  const [resolveError, setResolveError] = useState<string | null>(null)
  const [mediaError, setMediaError] = useState(false)
  const posterUrl = useMediaUrl(playback?.poster)

  const selectedSource = useMemo(() => (
    playback?.sources.find((source) => source.quality === selectedQuality) ?? null
  ), [playback, selectedQuality])
  const officialUrl = `https://hanime1.me/watch?v=${encodeURIComponent(chapterId)}`
  const hasError = resolveError !== null || mediaError

  useEffect(() => {
    const controller = new AbortController()
    setIsLoading(true)
    setResolveError(null)
    setMediaError(false)
    setPlayback(null)
    setSelectedQuality(null)
    resumeRef.current = null

    void resolveVideo(sourceId, chapterId, controller.signal)
      .then((resolved) => {
        if (controller.signal.aborted) return
        const initialSource = preferredSource(resolved.sources)
        if (!initialSource) {
          setResolveError("未找到可播放的视频画质")
          setIsLoading(false)
          return
        }
        setPlayback(resolved)
        setSelectedQuality(initialSource.quality)
        setIsLoading(false)
      })
      .catch((reason: unknown) => {
        if (controller.signal.aborted) return
        setResolveError(errorMessage(reason))
        setIsLoading(false)
      })

    return () => controller.abort()
  }, [attempt, chapterId, resolveVideo, sourceId])

  useEffect(() => {
    const dialog = dialogRef.current
    const previousFocus = document.activeElement instanceof HTMLElement
      ? document.activeElement
      : null
    const focusableElements = () => Array.from(
      dialog?.querySelectorAll<HTMLElement>(FOCUSABLE_SELECTOR) ?? [],
    ).filter((element) => !element.hidden && element.getAttribute("aria-hidden") !== "true")

    ;(focusableElements()[0] ?? dialog)?.focus()

    const onKeyDown = (event: KeyboardEvent) => {
      if (event.key === "Escape") {
        event.preventDefault()
        onClose()
        return
      }
      if (event.key !== "Tab" || !dialog) return

      const focusable = focusableElements()
      if (focusable.length === 0) {
        event.preventDefault()
        dialog.focus()
        return
      }
      const first = focusable[0]
      const last = focusable[focusable.length - 1]
      if (!first || !last) return
      if (event.shiftKey && (document.activeElement === first || !dialog.contains(document.activeElement))) {
        event.preventDefault()
        last.focus()
      } else if (!event.shiftKey && document.activeElement === last) {
        event.preventDefault()
        first.focus()
      }
    }

    document.addEventListener("keydown", onKeyDown)
    return () => {
      document.removeEventListener("keydown", onKeyDown)
      if (previousFocus?.isConnected) previousFocus.focus()
    }
  }, [onClose])

  const retry = useCallback(() => {
    setAttempt((value) => value + 1)
  }, [])

  const changeQuality = useCallback((quality: number) => {
    const video = videoRef.current
    resumeRef.current = video
      ? { time: video.currentTime, shouldResume: !video.paused }
      : null
    setMediaError(false)
    setSelectedQuality(quality)
  }, [])

  const restorePlayback = useCallback(() => {
    const video = videoRef.current
    const resume = resumeRef.current
    if (!video || !resume) return
    resumeRef.current = null
    video.currentTime = resume.time
    if (resume.shouldResume) {
      void video.play().catch(() => setMediaError(true))
    }
  }, [])

  return (
    <div
      ref={dialogRef}
      role="dialog"
      aria-modal="true"
      aria-labelledby="r18-video-player-title"
      tabIndex={-1}
      className={styles.videoBackdrop ?? ""}
    >
      <div className={styles.videoToolbar ?? ""}>
        <button
          type="button"
          onClick={onClose}
          aria-label="关闭播放器"
          className={styles.readerBackButton ?? ""}
        >
          ← 返回
        </button>
        <div id="r18-video-player-title" className={styles.videoTitle ?? ""}>
          {title}
        </div>
        {playback && selectedQuality !== null && (
          <label className={styles.videoQuality ?? ""}>
            <span>画质</span>
            <select
              aria-label="画质"
              value={selectedQuality}
              onChange={(event) => changeQuality(Number(event.target.value))}
            >
              {playback.sources.map((source) => (
                <option key={source.quality} value={source.quality}>
                  {source.quality}p
                </option>
              ))}
            </select>
          </label>
        )}
      </div>

      <div className={styles.videoStage ?? ""}>
        {isLoading && (
          <div role="status" className={styles.videoStatus ?? ""}>
            <Spinner size={30} />
            <span>正在解析视频地址</span>
          </div>
        )}

        {!isLoading && resolveError && (
          <div role="alert" className={styles.videoError ?? ""}>
            <strong>无法加载视频</strong>
            <span>{resolveError}</span>
            <div className={styles.videoErrorActions ?? ""}>
              <GlassButton variant="primary" onClick={retry}>重试</GlassButton>
              <a href={officialUrl} target="_blank" rel="noreferrer noopener">在官网打开</a>
            </div>
          </div>
        )}

        {!isLoading && playback && selectedSource && (
          <div className={styles.videoFrame ?? ""}>
            <video
              ref={videoRef}
              aria-label={`${title} 视频播放器`}
              src={selectedSource.url}
              poster={posterUrl || undefined}
              controls
              playsInline
              preload="metadata"
              onLoadedMetadata={restorePlayback}
              onError={() => setMediaError(true)}
              className={styles.videoElement ?? ""}
            />
            {mediaError && (
              <div role="alert" className={styles.videoMediaError ?? ""}>
                <strong>视频播放失败</strong>
                <span>可重试解析或切换其他画质。</span>
                <div className={styles.videoErrorActions ?? ""}>
                  <GlassButton variant="primary" onClick={retry}>重试</GlassButton>
                  <a href={officialUrl} target="_blank" rel="noreferrer noopener">在官网打开</a>
                </div>
              </div>
            )}
          </div>
        )}

        {!isLoading && !hasError && playback && !selectedSource && (
          <div role="alert" className={styles.videoError ?? ""}>当前画质不可用</div>
        )}
      </div>
    </div>
  )
}
