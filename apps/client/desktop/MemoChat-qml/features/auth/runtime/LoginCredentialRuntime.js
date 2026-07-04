.pragma library
const maxRecordCount = (maxCachedCredentials) => {
  const count = Number(maxCachedCredentials || 0);
  return count > 0 ? count : 0;
};
const normalizeCredential = (email) => ({
  email: String(email || "").trim()
});
const normalizeRecord = (item) => {
  if (!item) {
    return null;
  }
  const record = normalizeCredential(item.email);
  return record.email.length > 0 ? record : null;
};
const isSaveableCredential = (record) => !!record && record.email.length > 0;
const capCredentials = (records, maxCachedCredentials) => {
  const limit = maxRecordCount(maxCachedCredentials);
  const capped = [];
  for (let i = 0; i < records.length; ++i) {
    const record = normalizeRecord(records[i]);
    if (record) {
      capped.push(record);
      if (limit > 0 && capped.length >= limit) {
        break;
      }
    }
  }
  return capped;
};
const parseCredentialCache = (cacheJson, maxCachedCredentials) => {
  try {
    const parsed = JSON.parse(cacheJson || "[]");
    if (!Array.isArray(parsed)) {
      return [];
    }
    return capCredentials(parsed, maxCachedCredentials);
  } catch (e) {
    return [];
  }
};
const lastCredential = (records) => records && records.length > 0 ? normalizeRecord(records[0]) : null;
const credentialAt = (records, index) => {
  if (!records || index < 0 || index >= records.length) {
    return null;
  }
  return normalizeRecord(records[index]);
};
const buildSavedCredentials = (cacheJson, email, maxCachedCredentials) => {
  const saved = normalizeCredential(email);
  if (!isSaveableCredential(saved)) {
    return parseCredentialCache(cacheJson, maxCachedCredentials);
  }
  const savedEmail = saved.email.toLowerCase();
  const records = parseCredentialCache(cacheJson, maxCachedCredentials);
  const nextRecords = [saved];
  for (let i = 0; i < records.length; ++i) {
    const record = normalizeRecord(records[i]);
    if (record && record.email.toLowerCase() !== savedEmail) {
      nextRecords.push(record);
    }
  }
  return capCredentials(nextRecords, maxCachedCredentials);
};
