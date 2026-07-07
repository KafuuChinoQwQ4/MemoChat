/** Avatar — displays user avatar with a default image fallback */
import { useEffect, useState, type CSSProperties } from "react"
import { DEFAULT_AVATAR_DATA_URL } from "@/shared/media/mediaUrl"
import { useMediaUrl } from "@/shared/hooks/useMediaUrl"

export interface AvatarProps {
  /** Pass null or undefined to show initials fallback */
  src?: string | null | undefined
  name?: string | undefined
  size?: number
  className?: string
  style?: CSSProperties
  onClick?: () => void
}

export function Avatar({ src, name, size = 40, className, style, onClick }: AvatarProps) {
  const url = useMediaUrl(src)
  const [failedUrls, setFailedUrls] = useState<Set<string>>(() => new Set())
  const initials = name ? name.slice(0, 2).toUpperCase() : "?"
  const displayUrl = url && !failedUrls.has(url) ? url : DEFAULT_AVATAR_DATA_URL
  const showImage = Boolean(displayUrl && !failedUrls.has(displayUrl))

  useEffect(() => {
    setFailedUrls(new Set())
  }, [url])

  return (
    <div
      className={["avatar", className].filter(Boolean).join(" ")}
      onClick={onClick}
      style={{
        width: size,
        height: size,
        borderRadius: "50%",
        overflow: "hidden",
        flexShrink: 0,
        cursor: onClick ? "pointer" : undefined,
        backgroundColor: "var(--tint-hover)",
        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        fontSize: size * 0.4,
        fontWeight: 600,
        color: "var(--text-secondary)",
        ...style,
      }}
    >
      {showImage ? (
        <img
          src={displayUrl}
          alt={name ?? "avatar"}
          draggable={false}
          style={{ width: "100%", height: "100%", objectFit: "cover" }}
          onError={() => {
            setFailedUrls((current) => {
              const next = new Set(current)
              next.add(displayUrl)
              return next
            })
          }}
        />
      ) : (
        initials
      )}
    </div>
  )
}
