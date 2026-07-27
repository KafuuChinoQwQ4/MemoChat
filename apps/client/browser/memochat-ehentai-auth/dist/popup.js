"use strict";
// Popup script to display current transaction status
async function updateStatus() {
    try {
        const response = await chrome.runtime.sendMessage({
            type: 'MEMOCHAT_EHENTAI_GET_STATUS'
        });
        const statusDiv = document.getElementById('status');
        const statusText = document.getElementById('status-text');
        if (!statusDiv || !statusText)
            return;
        if (response.success && response.data) {
            const { status, message } = response.data;
            // Remove all status classes
            statusDiv.className = 'status';
            switch (status) {
                case 'waiting':
                case 'collecting':
                case 'validating':
                    statusDiv.classList.add('status-active');
                    statusText.textContent = message || 'Processing...';
                    break;
                case 'complete':
                    statusDiv.classList.add('status-success');
                    statusText.textContent = message || 'Login successful!';
                    break;
                case 'failed':
                case 'expired':
                    statusDiv.classList.add('status-error');
                    statusText.textContent = message || 'Login failed';
                    break;
                default:
                    statusDiv.classList.add('status-idle');
                    statusText.textContent = 'Idle - waiting for MemoChat';
            }
        }
        else {
            statusDiv.className = 'status status-idle';
            statusText.textContent = 'Idle - waiting for MemoChat';
        }
    }
    catch (err) {
        console.error('Failed to get status:', err);
    }
}
// Update status on load
updateStatus();
// Poll for status updates every 2 seconds
setInterval(updateStatus, 2000);
