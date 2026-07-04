.pragma library
const timeAgo = (ts) => {
  if (!ts) return "";
  const nowSec = Math.floor(Date.now() / 1e3);
  const diff = nowSec - Math.floor(ts / 1e3);
  if (diff < 60) return "\u521A\u521A";
  if (diff < 3600) return Math.floor(diff / 60) + "\u5206\u949F\u524D";
  if (diff < 86400) return Math.floor(diff / 3600) + "\u5C0F\u65F6\u524D";
  if (diff < 604800) return Math.floor(diff / 86400) + "\u5929\u524D";
  return new Date(ts).toLocaleDateString();
};
const videoDurationText = (durationMs) => {
  if (!durationMs || durationMs <= 0) return "\u70B9\u51FB\u6253\u5F00\u89C6\u9891";
  const totalSec = Math.floor(durationMs / 1e3);
  const minutes = Math.floor(totalSec / 60);
  const seconds = totalSec % 60;
  return minutes + ":" + (seconds < 10 ? "0" + seconds : seconds);
};
const itemType = (item) => String(item && (item.media_type || item.mediaType || item.type) || "").toLowerCase();
const itemMediaKey = (item) => String(item && (item.media_key || item.mediaKey || item.key) || "");
const itemContent = (item) => String(item && item.content || "");
const isMediaItem = (item) => {
  const type = itemType(item);
  return type === "image" || type === "video";
};
const imageMaxDim = (item) => {
  const w = item.width || 200;
  const h = item.height || 200;
  const aspect = w / (h || 1);
  return Math.min(320, Math.max(80, 200 * aspect));
};
const imageHeight = (item) => {
  const w = item.width || 200;
  const h = item.height || 200;
  const aspect = h / (w || 1);
  return Math.min(240, Math.max(60, 200 * aspect));
};
const mediaItemHeight = (item) => {
  if (!isMediaItem(item) || itemMediaKey(item).length === 0) return 0;
  if (itemType(item) === "video") return188;
  return imageHeight(item);
};
const mediaContentHeight = (items) => {
  if (!items || items.length === 0) return 0;
  let total = 0;
  let count = 0;
  for (let i = 0; i < items.length; i++) {
    const itemHeight = mediaItemHeight(items[i]);
    if (itemHeight <= 0) continue;
    if (count > 0) total += 8;
    total += itemHeight;
    count += 1;
  }
  return total;
};
const buildTextContent = (items) => {
  if (!items || items.length === 0) return "";
  const parts = [];
  for (let i = 0; i < items.length; i++) {
    const it = items[i];
    const content = itemContent(it);
    if (content.length > 0 && (!isMediaItem(it) || !itemMediaKey(it))) {
      parts.push(content);
    }
  }
  return parts.join("\n");
};
const containsMediaContent = (items) => {
  if (!items || items.length === 0) return false;
  for (let i = 0; i < items.length; i++) {
    const it = items[i];
    if (isMediaItem(it) && itemMediaKey(it).length > 0) return true;
  }
  return false;
};
const detailImgHeight = (item) => {
  const w = item.width || 200;
  const h = item.height || 200;
  return Math.min(160, Math.max(72, 120 * (h / (w || 1))));
};
const detailMediaTileHeight = (item) => {
  if (item.type === "video") return 124;
  return detailImgHeight(item);
};
