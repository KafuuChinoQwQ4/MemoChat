/**
 * R18PasswordField — password input with eye-toggle (show/hide).
 * Shared across all R18 account management panels.
 */
import { useState } from "react"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"

function EyeIcon({ open }: { open: boolean }) {
  return open ? (
    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
      <circle cx="12" cy="12" r="3"/>
    </svg>
  ) : (
    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94"/>
      <path d="M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19"/>
      <line x1="1" y1="1" x2="23" y2="23"/>
    </svg>
  )
}

interface R18PasswordFieldProps {
  value: string
  onChange: (e: React.ChangeEvent<HTMLInputElement>) => void
  placeholder?: string
  autoComplete?: string
  style?: React.CSSProperties
}

export function R18PasswordField({ value, onChange, placeholder, autoComplete, style }: R18PasswordFieldProps) {
  const [visible, setVisible] = useState(false)
  return (
    <div style={{ position: "relative", ...style }}>
      <GlassTextField
        type={visible ? "text" : "password"}
        value={value}
        onChange={onChange}
        placeholder={placeholder}
        autoComplete={autoComplete}
        style={{ paddingRight: 40 }}
      />
      <button
        type="button"
        onClick={() => setVisible((v) => !v)}
        title={visible ? "隐藏密码" : "显示密码"}
        aria-label={visible ? "隐藏密码" : "显示密码"}
        style={{
          position: "absolute",
          right: 10,
          top: "50%",
          transform: "translateY(-50%)",
          background: "none",
          border: "none",
          cursor: "pointer",
          padding: 4,
          color: "var(--text-secondary)",
          display: "flex",
          alignItems: "center",
          lineHeight: 0,
        }}
        tabIndex={-1}
      >
        <EyeIcon open={visible} />
      </button>
    </div>
  )
}
