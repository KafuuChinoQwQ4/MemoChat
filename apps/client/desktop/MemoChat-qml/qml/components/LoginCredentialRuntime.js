.pragma library

function maxRecordCount(maxCachedCredentials) {
    var count = Number(maxCachedCredentials || 0)
    return count > 0 ? count : 0
}

function normalizeCredential(email, password) {
    return {
        "email": String(email || "").trim(),
        "password": String(password || "")
    }
}

function normalizeRecord(item) {
    if (!item) {
        return null
    }
    var record = normalizeCredential(item.email, item.password)
    return record.email.length > 0 ? record : null
}

function isSaveableCredential(record) {
    return !!record && record.email.length > 0 && record.password.length > 0
}

function capCredentials(records, maxCachedCredentials) {
    var limit = maxRecordCount(maxCachedCredentials)
    var capped = []
    for (var i = 0; i < records.length; ++i) {
        var record = normalizeRecord(records[i])
        if (record) {
            capped.push(record)
            if (limit > 0 && capped.length >= limit) {
                break
            }
        }
    }
    return capped
}

function parseCredentialCache(cacheJson, maxCachedCredentials) {
    try {
        var parsed = JSON.parse(cacheJson || "[]")
        if (!Array.isArray(parsed)) {
            return []
        }
        return capCredentials(parsed, maxCachedCredentials)
    } catch (e) {
        return []
    }
}

function lastCredential(records) {
    return records && records.length > 0 ? normalizeRecord(records[0]) : null
}

function credentialAt(records, index) {
    if (!records || index < 0 || index >= records.length) {
        return null
    }
    return normalizeRecord(records[index])
}

function buildSavedCredentials(cacheJson, email, password, maxCachedCredentials) {
    var saved = normalizeCredential(email, password)
    if (!isSaveableCredential(saved)) {
        return parseCredentialCache(cacheJson, maxCachedCredentials)
    }

    var savedEmail = saved.email.toLowerCase()
    var records = parseCredentialCache(cacheJson, maxCachedCredentials)
    var nextRecords = [saved]
    for (var i = 0; i < records.length; ++i) {
        var record = normalizeRecord(records[i])
        if (record && record.email.toLowerCase() !== savedEmail) {
            nextRecords.push(record)
        }
    }
    return capCredentials(nextRecords, maxCachedCredentials)
}
