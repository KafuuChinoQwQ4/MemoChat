// Background service worker for MemoChat E-Hentai authentication
// Manages the login transaction, cookie observation, and completion

interface ImportTransaction {
  importId: string;
  ticket: string;
  memochatOrigin: string;
  loginTabId: number | null;
  status: 'waiting' | 'collecting' | 'validating' | 'complete' | 'failed' | 'expired';
  message: string;
  expiresAt: number;
  collectedCookies: {
    ipb_member_id?: string;
    ipb_pass_hash?: string;
    igneous?: string;
    sk?: string;
  };
}

let activeTransaction: ImportTransaction | null = null;

const ALLOWED_COOKIE_NAMES = new Set(['ipb_member_id', 'ipb_pass_hash', 'igneous', 'sk']);
const EHENTAI_DOMAINS = ['.e-hentai.org', 'e-hentai.org', 'exhentai.org', '.exhentai.org'];

// Listen for messages from content script
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.type === 'MEMOCHAT_EHENTAI_START_IMPORT') {
    handleStartImport(message.payload)
      .then(result => sendResponse({ success: true, data: result }))
      .catch(error => sendResponse({ success: false, error: error.message }));
    return true; // Async response
  }

  if (message.type === 'MEMOCHAT_EHENTAI_GET_STATUS') {
    sendResponse({
      success: true,
      data: activeTransaction ? {
        status: activeTransaction.status,
        message: activeTransaction.message
      } : null
    });
    return false;
  }

  if (message.type === 'MEMOCHAT_EHENTAI_CANCEL') {
    cancelTransaction();
    sendResponse({ success: true });
    return false;
  }
});

async function handleStartImport(payload: {
  importId: string;
  ticket: string;
  memochatOrigin: string;
  expiresAt: number;
}): Promise<void> {
  // Cancel any existing transaction
  if (activeTransaction) {
    cancelTransaction();
  }

  // Validate MemoChat origin (only localhost/127.0.0.1)
  const origin = new URL(payload.memochatOrigin);
  if (!['localhost', '127.0.0.1'].includes(origin.hostname)) {
    throw new Error('Invalid MemoChat origin');
  }

  // Create new transaction
  activeTransaction = {
    importId: payload.importId,
    ticket: payload.ticket,
    memochatOrigin: payload.memochatOrigin,
    loginTabId: null,
    status: 'waiting',
    message: 'Opening E-Hentai forum login page...',
    expiresAt: payload.expiresAt,
    collectedCookies: {}
  };

  // Set expiry timeout
  const timeoutMs = payload.expiresAt - Date.now();
  if (timeoutMs > 0) {
    setTimeout(() => {
      if (activeTransaction && activeTransaction.importId === payload.importId) {
        activeTransaction.status = 'expired';
        activeTransaction.message = 'Login timeout expired';
      }
    }, timeoutMs);
  }

  // Open E-Hentai forum login page
  const tab = await chrome.tabs.create({
    url: 'https://forums.e-hentai.org/index.php?act=Login',
    active: true
  });

  if (tab.id) {
    activeTransaction.loginTabId = tab.id;
    startCookieObservation();
  }
}

function startCookieObservation() {
  if (!activeTransaction) return;

  // Listen for cookie changes
  chrome.cookies.onChanged.addListener(handleCookieChange);

  // Also check existing cookies
  checkExistingCookies();
}

function handleCookieChange(changeInfo: chrome.cookies.CookieChangeInfo) {
  if (!activeTransaction || activeTransaction.status !== 'waiting') return;

  const cookie = changeInfo.cookie;
  if (!cookie || changeInfo.removed) return;

  // Check if this is an allowed E-Hentai cookie
  if (!ALLOWED_COOKIE_NAMES.has(cookie.name)) return;
  if (!EHENTAI_DOMAINS.some(domain => cookie.domain.includes(domain))) return;

  // Store the cookie value (never log it)
  activeTransaction.collectedCookies[cookie.name as keyof typeof activeTransaction.collectedCookies] = cookie.value;

  // Check if we have the required cookies
  checkCompleteness();
}

async function checkExistingCookies() {
  if (!activeTransaction) return;

  for (const domain of EHENTAI_DOMAINS) {
    try {
      const cookies = await chrome.cookies.getAll({ domain });
      for (const cookie of cookies) {
        if (ALLOWED_COOKIE_NAMES.has(cookie.name)) {
          activeTransaction.collectedCookies[cookie.name as keyof typeof activeTransaction.collectedCookies] = cookie.value;
        }
      }
    } catch (err) {
      console.error('Failed to check cookies for domain', domain, err);
    }
  }

  checkCompleteness();
}

function checkCompleteness() {
  if (!activeTransaction || activeTransaction.status !== 'waiting') return;

  const { ipb_member_id, ipb_pass_hash } = activeTransaction.collectedCookies;

  // Require both ipb_member_id and ipb_pass_hash
  if (ipb_member_id && ipb_pass_hash) {
    activeTransaction.status = 'collecting';
    activeTransaction.message = 'Cookies collected, verifying access...';

    // Wait a moment for igneous cookie (ExHentai)
    setTimeout(() => {
      if (activeTransaction && activeTransaction.status === 'collecting') {
        completeImport();
      }
    }, 2000);
  }
}

async function completeImport() {
  if (!activeTransaction) return;

  activeTransaction.status = 'validating';
  activeTransaction.message = 'Submitting to MemoChat...';

  try {
    const response = await fetch(`${activeTransaction.memochatOrigin}/api/r18/account/browser-import/complete`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        ticket: activeTransaction.ticket,
        cookies: activeTransaction.collectedCookies
      })
    });

    const result = await response.json();

    if (response.ok && result.ok && result.data?.success) {
      activeTransaction.status = 'complete';
      activeTransaction.message = result.data.message || 'Login successful!';

      // Close the login tab after a brief delay
      if (activeTransaction.loginTabId) {
        setTimeout(() => {
          if (activeTransaction?.loginTabId) {
            chrome.tabs.remove(activeTransaction.loginTabId);
          }
        }, 2000);
      }
    } else {
      activeTransaction.status = 'failed';
      activeTransaction.message = result.data?.message || result.message || 'Import failed';
    }
  } catch (err) {
    activeTransaction.status = 'failed';
    activeTransaction.message = err instanceof Error ? err.message : 'Network error';
  } finally {
    // Stop observing cookies
    chrome.cookies.onChanged.removeListener(handleCookieChange);
  }
}

function cancelTransaction() {
  if (activeTransaction) {
    chrome.cookies.onChanged.removeListener(handleCookieChange);
    if (activeTransaction.loginTabId) {
      chrome.tabs.remove(activeTransaction.loginTabId).catch(() => {});
    }
    activeTransaction = null;
  }
}

// Clean up on tab close
chrome.tabs.onRemoved.addListener((tabId) => {
  if (activeTransaction && activeTransaction.loginTabId === tabId) {
    activeTransaction.status = 'failed';
    activeTransaction.message = 'Login tab closed';
    chrome.cookies.onChanged.removeListener(handleCookieChange);
  }
});

export {};
