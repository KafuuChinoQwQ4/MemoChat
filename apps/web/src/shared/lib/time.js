/** Time formatting utilities */
const rtf = new Intl.RelativeTimeFormat("zh-CN", { numeric: "auto" });
export function formatMessageTime(timestamp) {
    const now = Date.now();
    const diff = now - timestamp;
    if (diff < 60000)
        return "刚刚";
    if (diff < 3600000)
        return `${Math.floor(diff / 60000)} 分钟前`;
    const d = new Date(timestamp);
    const today = new Date();
    if (d.toDateString() === today.toDateString()) {
        return d.toLocaleTimeString("zh-CN", { hour: "2-digit", minute: "2-digit" });
    }
    const yesterday = new Date(today);
    yesterday.setDate(today.getDate() - 1);
    if (d.toDateString() === yesterday.toDateString())
        return "昨天";
    return d.toLocaleDateString("zh-CN", { month: "numeric", day: "numeric" });
}
export function formatFullTime(timestamp) {
    return new Date(timestamp).toLocaleString("zh-CN", {
        year: "numeric", month: "2-digit", day: "2-digit",
        hour: "2-digit", minute: "2-digit",
    });
}
export const _ = rtf; // suppress unused import warning
