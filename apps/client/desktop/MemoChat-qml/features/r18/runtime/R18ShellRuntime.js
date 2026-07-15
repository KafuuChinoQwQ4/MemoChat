.pragma library
function absoluteUrl(url, gatewayBaseUrl) {
  if (!url) {
    return "";
  }
  if (url.indexOf("http://") === 0 || url.indexOf("https://") === 0 || url.indexOf("qrc:/") === 0) {
    return url;
  }
  if (url.indexOf("/") === 0 && gatewayBaseUrl.length > 0) {
    return gatewayBaseUrl + url;
  }
  return url;
}
function normalizeTagArray(tags) {
  if (!tags) {
    return [];
  }
  if (typeof tags === "string") {
    return tags.length > 0 ? [tags] : [];
  }
  if (!Array.isArray(tags)) {
    return [];
  }
  const values = [];
  for (let i = 0; i < tags.length; ++i) {
    const tag = String(tags[i] || "").trim();
    if (tag.length > 0) {
      values.push(tag);
    }
  }
  return values;
}
function buildSourceTagBuckets(model) {
  if (!model) {
    return [];
  }
  const counts = {};
  const total = model.count !== void 0 ? model.count : 0;
  for (let i = 0; i < total; ++i) {
    const item = model.get(i);
    if (!item) {
      continue;
    }
    const tags = normalizeTagArray(item.tags);
    for (let j = 0; j < tags.length; ++j) {
      const key = tags[j];
      if (!counts[key]) {
        counts[key] = 0;
      }
      counts[key] += 1;
    }
  }
  const buckets = [];
  for (const name in counts) {
    buckets.push({ name, count: counts[name], id: name });
  }
  buckets.sort((a, b) => {
    if (b.count !== a.count) {
      return b.count - a.count;
    }
    return a.name.localeCompare(b.name);
  });
  return buckets;
}
function sourceFilterConfig(sourceId) {
  const sid = String(sourceId || "");
  const empty = {
    sorts: [{ id: "", label: "\u9ED8\u8BA4" }],
    tags: [{ id: "", label: "\u5168\u90E8" }],
    tagPlaceholder: "tag / \u5206\u7C7B"
  };
  if (sid === "jm.official") {
    return {
      sorts: [
        { id: "mr", label: "\u6700\u65B0" },
        { id: "mv_t", label: "\u65E5\u6392\u884C" },
        { id: "mv_w", label: "\u9031\u6392\u884C" },
        { id: "mv_m", label: "\u6708\u6392\u884C" },
        { id: "mv", label: "\u7E3D\u6392\u884C" },
        { id: "mp", label: "\u6700\u591A\u9EDE\u8D0A" },
        { id: "ld", label: "\u6700\u591A\u611B\u5FC3" },
        { id: "tf", label: "\u6700\u591A\u5716\u7247" }
      ],
      tags: [
        { id: "", label: "\u5168\u90E8" },
        { id: "\u540C\u4EBA", label: "\u540C\u4EBA" },
        { id: "\u55AE\u672C", label: "\u55AE\u672C" },
        { id: "\u77ED\u7BC7", label: "\u77ED\u7BC7" },
        { id: "\u5176\u4ED6\u985E", label: "\u5176\u4ED6\u985E" },
        { id: "\u97D3\u6F2B", label: "\u97D3\u6F2B" },
        { id: "English Manga", label: "English Manga" },
        { id: "\u4E00\u822C\u5411\u97D3\u6F2B", label: "\u4E00\u822C\u5411\u97D3\u6F2B" },
        { id: "Cosplay", label: "Cosplay" },
        { id: "3D", label: "3D" },
        { id: "\u7981\u6F2B\u6F22\u5316\u7D44", label: "\u7981\u6F2B\u6F22\u5316\u7D44" },
        { id: "\u6F22\u5316", label: "\u6F22\u5316" },
        { id: "\u65E5\u8A9E", label: "\u65E5\u8A9E" },
        { id: "CG\u5716\u96C6", label: "CG\u5716\u96C6" },
        { id: "\u7981\u6F2B\u53BB\u78BC", label: "\u7981\u6F2B\u53BB\u78BC" },
        { id: "\u7981\u6F2B\u4E0A\u8272", label: "\u7981\u6F2B\u4E0A\u8272" },
        { id: "\u9752\u5E74\u6F2B", label: "\u9752\u5E74\u6F2B" },
        { id: "\u5287\u60C5\u5411", label: "\u5287\u60C5\u5411" },
        { id: "\u6821\u5712", label: "\u6821\u5712" },
        { id: "\u7D14\u611B", label: "\u7D14\u611B" },
        { id: "\u4EBA\u59BB", label: "\u4EBA\u59BB" },
        { id: "\u5E2B\u751F", label: "\u5E2B\u751F" },
        { id: "\u8FD1\u89AA", label: "\u8FD1\u89AA" },
        { id: "\u767E\u5408", label: "\u767E\u5408" },
        { id: "YAOI", label: "YAOI" },
        { id: "\u6027\u8F49", label: "\u6027\u8F49" },
        { id: "NTR", label: "NTR" },
        { id: "\u507D\u5A18", label: "\u507D\u5A18" },
        { id: "\u7661\u5973", label: "\u7661\u5973" },
        { id: "\u5168\u5F69", label: "\u5168\u5F69" },
        { id: "\u5973\u6027\u5411", label: "\u5973\u6027\u5411" },
        { id: "\u863F\u8389", label: "\u863F\u8389" },
        { id: "\u79A6\u59D0", label: "\u79A6\u59D0" },
        { id: "\u719F\u5973", label: "\u719F\u5973" },
        { id: "\u6B63\u592A", label: "\u6B63\u592A" },
        { id: "\u5DE8\u4E73", label: "\u5DE8\u4E73" },
        { id: "\u8CA7\u4E73", label: "\u8CA7\u4E73" },
        { id: "\u5973\u738B", label: "\u5973\u738B" },
        { id: "\u6559\u5E2B", label: "\u6559\u5E2B" },
        { id: "\u5973\u50D5", label: "\u5973\u50D5" },
        { id: "\u8B77\u58EB", label: "\u8B77\u58EB" },
        { id: "\u6CF3\u88DD", label: "\u6CF3\u88DD" },
        { id: "\u773C\u93E1", label: "\u773C\u93E1" },
        { id: "\u9023\u8932\u896A", label: "\u9023\u8932\u896A" },
        { id: "\u5154\u5973\u90CE", label: "\u5154\u5973\u90CE" },
        { id: "\u7FA4\u4EA4", label: "\u7FA4\u4EA4" },
        { id: "\u8DB3\u4EA4", label: "\u8DB3\u4EA4" },
        { id: "SM", label: "SM" },
        { id: "\u809B\u4EA4", label: "\u809B\u4EA4" },
        { id: "\u963F\u9ED1\u984F", label: "\u963F\u9ED1\u984F" },
        { id: "\u85E5\u7269", label: "\u85E5\u7269" },
        { id: "\u6276\u4ED6", label: "\u6276\u4ED6" },
        { id: "\u8ABF\u6559", label: "\u8ABF\u6559" },
        { id: "\u91CE\u5916\u9732\u51FA", label: "\u91CE\u5916\u9732\u51FA" },
        { id: "\u50AC\u7720", label: "\u50AC\u7720" },
        { id: "\u81EA\u6170", label: "\u81EA\u6170" },
        { id: "\u89F8\u624B", label: "\u89F8\u624B" },
        { id: "\u7378\u4EA4", label: "\u7378\u4EA4" },
        { id: "CG\u96C6", label: "CG\u96C6" },
        { id: "\u91CD\u53E3", label: "\u91CD\u53E3" },
        { id: "\u7375\u5947", label: "\u7375\u5947" },
        { id: "\u975EH", label: "\u975EH" },
        { id: "\u8840\u8165\u66B4\u529B", label: "\u8840\u8165\u66B4\u529B" }
      ],
      tagPlaceholder: "\u5B98\u65B9\u7E41\u9AD4 tag\uFF0C\u4F8B\u5982 \u540C\u4EBA\u3001\u6F22\u5316"
    };
  }
  if (sid === "picacg.official") {
    return {
      sorts: [
        { id: "dd", label: "\u65B0\u5230\u820A" },
        { id: "da", label: "\u820A\u5230\u65B0" },
        { id: "ld", label: "\u6700\u591A\u611B\u5FC3" },
        { id: "vd", label: "\u6700\u591A\u6307\u540D" }
      ],
      tags: [
        { id: "", label: "\u5168\u90E8" },
        { id: "\u5927\u5BB6\u90FD\u5728\u770B", label: "\u5927\u5BB6\u90FD\u5728\u770B" },
        { id: "\u5927\u6FD5\u63A8\u85A6", label: "\u5927\u6FD5\u63A8\u85A6" },
        { id: "\u5168\u5F69", label: "\u5168\u5F69" },
        { id: "\u9577\u7BC7", label: "\u9577\u7BC7" },
        { id: "\u77ED\u7BC7", label: "\u77ED\u7BC7" },
        { id: "\u5B8C\u7D50", label: "\u5B8C\u7D50" },
        { id: "\u540C\u4EBA", label: "\u540C\u4EBA" },
        { id: "\u55AE\u884C\u672C", label: "\u55AE\u884C\u672C" },
        { id: "\u97D3\u6F2B", label: "\u97D3\u6F2B" },
        { id: "\u7F8E\u6F2B", label: "\u7F8E\u6F2B" },
        { id: "\u751F\u8089", label: "\u751F\u8089" },
        { id: "\u6F22\u5316", label: "\u6F22\u5316" },
        { id: "\u7D14\u611B", label: "\u7D14\u611B" },
        { id: "\u5F8C\u5BAE", label: "\u5F8C\u5BAE" },
        { id: "\u767E\u5408", label: "\u767E\u5408" },
        { id: "\u803D\u7F8E", label: "\u803D\u7F8E" },
        { id: "\u507D\u5A18", label: "\u507D\u5A18" },
        { id: "\u6276\u4ED6", label: "\u6276\u4ED6" },
        { id: "\u6027\u8F49\u63DB", label: "\u6027\u8F49\u63DB" },
        { id: "NTR", label: "NTR" },
        { id: "\u91CD\u53E3", label: "\u91CD\u53E3" },
        { id: "\u7375\u5947", label: "\u7375\u5947" },
        { id: "\u6EAB\u99A8", label: "\u6EAB\u99A8" },
        { id: "\u5287\u60C5", label: "\u5287\u60C5" },
        { id: "\u6821\u5712", label: "\u6821\u5712" },
        { id: "\u5DE8\u4E73", label: "\u5DE8\u4E73" },
        { id: "\u8CA7\u4E73", label: "\u8CA7\u4E73" },
        { id: "\u4EBA\u59BB", label: "\u4EBA\u59BB" },
        { id: "\u8ABF\u6559", label: "\u8ABF\u6559" },
        { id: "\u89F8\u624B", label: "\u89F8\u624B" },
        { id: "\u963F\u9ED1\u984F", label: "\u963F\u9ED1\u984F" }
      ],
      tagPlaceholder: "\u5B98\u65B9\u7E41\u9AD4\u5206\u985E\uFF0C\u4F8B\u5982 \u5168\u5F69\u3001\u9577\u7BC7"
    };
  }
  if (sid === "nhentai.official") {
    return {
      sorts: [
        { id: "", label: "Recent" },
        { id: "popular-today", label: "Popular Today" },
        { id: "popular-week", label: "Popular Week" },
        { id: "popular-month", label: "Popular Month" },
        { id: "popular", label: "Popular All-time" }
      ],
      tags: [
        { id: "", label: "All" },
        { id: "language:chinese", label: "Chinese" },
        { id: "language:japanese", label: "Japanese" },
        { id: "language:english", label: "English" },
        { id: "language:translated", label: "Translated" },
        { id: "category:doujinshi", label: "Doujinshi" },
        { id: "category:manga", label: "Manga" },
        { id: "full color", label: "full color" },
        { id: "big breasts", label: "big breasts" },
        { id: "sole female", label: "sole female" },
        { id: "group", label: "group" },
        { id: "yuri", label: "yuri" },
        { id: "yaoi", label: "yaoi" },
        { id: "futanari", label: "futanari" },
        { id: "netorare", label: "netorare" },
        { id: "ahegao", label: "ahegao" },
        { id: "uncensored", label: "uncensored" }
      ],
      tagPlaceholder: "language:chinese / sole female"
    };
  }
  if (sid === "ehentai.official" || sid === "exhentai.official") {
    return {
      sorts: [
        { id: "", label: "All Categories" },
        { id: "doujinshi", label: "Doujinshi" },
        { id: "manga", label: "Manga" },
        { id: "artist_cg", label: "Artist CG" },
        { id: "game_cg", label: "Game CG" },
        { id: "western", label: "Western" },
        { id: "non-h", label: "Non-H" },
        { id: "image_set", label: "Image Set" },
        { id: "cosplay", label: "Cosplay" },
        { id: "asian_porn", label: "Asian Porn" },
        { id: "misc", label: "Misc" }
      ],
      tags: [
        { id: "", label: "No extra tag" },
        { id: "language:chinese", label: "language:chinese" },
        { id: "language:japanese", label: "language:japanese" },
        { id: "language:english", label: "language:english" },
        { id: "other:full color", label: "other:full color" },
        { id: "other:uncensored", label: "other:uncensored" },
        { id: "female:big breasts", label: "female:big breasts" },
        { id: "female:sole female", label: "female:sole female" },
        { id: "female:ahegao", label: "female:ahegao" },
        { id: "female:netorare", label: "female:netorare" },
        { id: "female:yuri", label: "female:yuri" },
        { id: "male:yaoi", label: "male:yaoi" }
      ],
      tagPlaceholder: "language:chinese / female:big breasts"
    };
  }
  return empty;
}
function defaultSortForSource(sourceId) {
  var _a;
  const cfg = sourceFilterConfig(sourceId);
  return ((_a = cfg.sorts[0]) == null ? void 0 : _a.id) || "";
}
function mergeOfficialAndDerivedTags(sourceId, derivedBuckets) {
  const cfg = sourceFilterConfig(sourceId);
  const official = (cfg.tags || []).filter((t) => t && t.id).map((t) => ({ id: t.id, name: t.label || t.id, count: 0, official: true }));
  const seen = {};
  for (const o of official) {
    seen[o.id] = true;
  }
  for (const d of derivedBuckets || []) {
    const key = d.name || d.id || "";
    if (!key || seen[key]) continue;
    official.push({ id: key, name: key, count: d.count || 0, official: false });
    seen[key] = true;
  }
  return official;
}
function isGridAtBottom(gridView, threshold) {
  if (!gridView) {
    return false;
  }
  return gridView.contentHeight <= gridView.height || gridView.contentY + gridView.height >= gridView.contentHeight - threshold;
}
function modelCount(model) {
  return model && model.count !== void 0 ? model.count : 0;
}
function currentComicId(currentComic) {
  if (!currentComic) {
    return "";
  }
  return currentComic.comic_id || currentComic.id || "";
}
function currentComicTitle(currentComic) {
  if (!currentComic) {
    return "";
  }
  return currentComic.title || currentComic.name || "";
}
