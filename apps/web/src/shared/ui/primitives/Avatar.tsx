/** Avatar — displays user avatar with fallback initials */
import { useMediaUrl } from "@/shared/hooks/useMediaUrl"
import { cn } from "@/shared/lib/classnames"

export interface AvatarProps {
  /** Pass null or undefined to show initials fallback */
  src?: string | null | undefined
  name?: string | undefined
  size?: number
  className?: string
  style?: React.CSSProperties
  onClick?: () => void
}

export function Avatar({ src, name, size = 40, className, onClick }: AvatarProps) {
  const url = useMediaUrl(src)
  const initials = name ? name.slice(0, 2).toUpperCase() : "?"

  return (
    <div
      className={cn("avatar", className)}
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
      }}
    >
      {url ? (
        <img
          src={url}
          alt={name}
          style={{ width: "100%", height: "100%", objectFit: "cover" }}
          onError={(e) => { (e.currentTarget as HTMLImageElement).style.display = "none" }}
        />
      ) : (
        initials
      )}
    </div>
  )
}
