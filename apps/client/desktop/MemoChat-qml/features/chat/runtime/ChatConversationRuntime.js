.pragma library
const clampValue = (value, lower, upper) => {
  return Math.max(lower, Math.min(upper, value));
};
const parseSuggestionOptions = (result) => {
  const text = result || "";
  const options = [];
  try {
    const parsed = JSON.parse(text);
    if (Array.isArray(parsed)) {
      for (let i = 0; i < parsed.length && options.length < 3; ++i) {
        const item = String(parsed[i]).trim();
        if (item.length > 0) {
          options.push(item);
        }
      }
    }
  } catch (e) {
  }
  if (options.length === 0) {
    const lines = text.split(/\r?\n/);
    for (let j = 0; j < lines.length && options.length < 3; ++j) {
      const line = lines[j].replace(/^[\s\-*•\d.、)）]+/, "").trim();
      if (line.length > 0) {
        options.push(line);
      }
    }
  }
  while (options.length < 3) {
    options.push(
      options.length === 0 ? "\u597D\u7684\uFF0C\u6211\u770B\u4E00\u4E0B\u3002" : options.length === 1 ? "\u6536\u5230\uFF0C\u6211\u7A0D\u540E\u56DE\u590D\u3002" : "\u53EF\u4EE5\uFF0C\u6211\u4EEC\u7EE7\u7EED\u804A\u3002"
    );
  }
  return options.slice(0, 3);
};
