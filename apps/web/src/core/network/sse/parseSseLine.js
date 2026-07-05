export function parseSseLine(line) {
    if (line.startsWith(":") || line.trim() === "")
        return null;
    const colon = line.indexOf(":");
    if (colon === -1)
        return { field: line, value: "" };
    const field = line.slice(0, colon);
    const value = line.slice(colon + 1).replace(/^ /, "");
    return { field, value };
}
