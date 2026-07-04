export const clampValue = (value: number, lower: number, upper: number): number => {
    return Math.max(lower, Math.min(upper, value));
};

export const parseSuggestionOptions = (result: string | null | undefined): string[] => {
    const text: string = result || "";
    const options: string[] = [];

    try {
        const parsed: unknown = JSON.parse(text);
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
            options.length === 0
                ? "好的，我看一下。"
                : options.length === 1
                ? "收到，我稍后回复。"
                : "可以，我们继续聊。"
        );
    }

    return options.slice(0, 3);
};
