# MemoChat E-Hentai Browser Authentication Extension

## Overview

This Manifest V3 browser extension provides secure E-Hentai/ExHentai authentication for MemoChat without requiring password storage in MemoChat itself.

## Architecture

- **Background Service Worker** (`background.ts`): Manages the login transaction, observes E-Hentai cookies, and submits them to MemoChat via capability-authenticated API
- **Content Script** (`content.ts`): Bridge between MemoChat web page and extension, validates origin restrictions
- **Popup UI** (`popup.html/ts`): Displays current transaction status

## Security Features

1. **No Password Exposure**: User enters credentials only on official E-Hentai forum page
2. **Origin Restrictions**: Only accepts messages from `localhost` or `127.0.0.1`
3. **Host Permissions**: Limited to E-Hentai domains and local MemoChat origins
4. **Cookie Allowlist**: Only collects `ipb_member_id`, `ipb_pass_hash`, `igneous`, `sk`
5. **Capability Authentication**: Extension submits to `/api/r18/account/browser-import/complete` using one-time ticket (no JWT required)
6. **Transaction Cleanup**: Cookies never logged, transaction state cleared on completion/failure

## Installation (Development)

1. Build the extension:
   ```bash
   cd apps/client/browser/memochat-ehentai-auth
   npm install
   npm run build
   ```

2. Load unpacked extension in Chrome/Edge:
   - Navigate to `chrome://extensions/`
   - Enable "Developer mode"
   - Click "Load unpacked"
   - Select the `dist` directory

## Usage Flow

1. MemoChat web page detects extension via content script bridge
2. User clicks "网页登录" button in MemoChat R18 account settings
3. MemoChat calls backend `/api/r18/account/browser-import/start` to get ticket
4. MemoChat sends `EHENTAI_START_IMPORT` message to extension with ticket
5. Extension opens `https://forums.e-hentai.org/index.php?act=Login` in new tab
6. User completes login on official page (handles Cloudflare challenges)
7. Extension observes cookie changes via `chrome.cookies.onChanged`
8. Once required cookies collected, extension POSTs to `/api/r18/account/browser-import/complete`
9. Backend validates session, imports to both `ehentai.official` and `exhentai.official`
10. Extension closes login tab and notifies MemoChat

## Development Notes

- TypeScript compilation outputs to `dist/` directory
- `build.js` copies static assets (manifest, HTML, placeholder icons)
- Production deployment should include proper icon assets
- Extension never stores cookies persistently or exposes them via `window.postMessage`

## Testing

Manual testing required:
- Verify extension detection in MemoChat web
- Test complete login flow with valid E-Hentai account
- Test timeout/cancellation paths
- Verify login tab closes after successful import
- Check ExHentai access detection with igneous cookie

## Production Considerations

- Replace placeholder icons with actual branded icons
- Consider publishing to Chrome Web Store (requires verification)
- Document extension installation instructions for end users
- Add extension update mechanism
