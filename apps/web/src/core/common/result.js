export function ok(value) {
    return { ok: true, value };
}
export function err(error) {
    return { ok: false, error };
}
