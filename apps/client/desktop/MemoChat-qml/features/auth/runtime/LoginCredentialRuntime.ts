export interface Credential {
    email: string;
}

export const maxRecordCount = (maxCachedCredentials: number | string | null | undefined): number => {
    const count = Number(maxCachedCredentials || 0);
    return count > 0 ? count : 0;
};

export const normalizeCredential = (email: string | null | undefined): Credential => ({
    email: String(email || "").trim(),
});

export const normalizeRecord = (item: { email?: string | null | undefined } | null | undefined): Credential | null => {
    if (!item) {
        return null;
    }
    const record = normalizeCredential(item.email);
    return record.email.length > 0 ? record : null;
};

export const isSaveableCredential = (record: Credential | null | undefined): boolean =>!!record && record.email.length > 0;

export const capCredentials = (records: unknown[], maxCachedCredentials: number | string | null | undefined): Credential[] => {
    const limit = maxRecordCount(maxCachedCredentials);
    const capped: Credential[] = [];
    for (let i = 0; i < records.length; ++i) {
        const record = normalizeRecord(records[i] as { email?: string | null | undefined } | null | undefined);
        if (record) {
            capped.push(record);
            if (limit > 0 && capped.length >= limit) {
                break;
            }
        }
    }
    return capped;
};

export const parseCredentialCache = (cacheJson: string | null | undefined, maxCachedCredentials: number | string | null | undefined): Credential[] => {
    try {
        const parsed: unknown = JSON.parse(cacheJson || "[]");
        if (!Array.isArray(parsed)) {
            return [];
        }
        return capCredentials(parsed, maxCachedCredentials);
    } catch (e) {
        return [];
    }
};

export const lastCredential = (records: unknown[] | null | undefined): Credential | null =>
    records && records.length > 0 ? normalizeRecord(records[0] as { email?: string | null | undefined }) : null;

export const credentialAt = (records: unknown[] | null | undefined, index: number): Credential | null => {
    if (!records || index < 0 || index >= records.length) {
        return null;
    }
    return normalizeRecord(records[index] as { email?: string | null | undefined });
};

export const buildSavedCredentials = (cacheJson: string | null | undefined, email: string | null | undefined, maxCachedCredentials: number | string | null | undefined): Credential[] => {
    const saved = normalizeCredential(email);
    if (!isSaveableCredential(saved)) {
        return parseCredentialCache(cacheJson, maxCachedCredentials);
    }

    const savedEmail = saved.email.toLowerCase();
    const records = parseCredentialCache(cacheJson, maxCachedCredentials);
    const nextRecords: Credential[] = [saved];
    for (let i = 0; i < records.length; ++i) {
        const record = normalizeRecord(records[i]);
        if (record && record.email.toLowerCase() !== savedEmail) {
            nextRecords.push(record);
        }
    }
    return capCredentials(nextRecords, maxCachedCredentials);
};
