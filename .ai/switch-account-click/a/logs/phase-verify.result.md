# Verification

Commands run:

- `cmake --preset msvc2022-client-verify`: PASS. Configure completed; Qt reported existing `WrapVulkanHeaders` missing messages.
- `cmake --build --preset msvc2022-client-verify`: PASS. `MemoChatQml.exe` linked successfully.
- `cmake --build --preset msvc2022-full --target MemoChatQml`: PASS. Main `build\bin\Release\MemoChatQml.exe` updated.
- Copied `build\bin\Release\MemoChatQml.exe` to `infra\Memo_ops\runtime\services\MemoChatQml\MemoChatQml.exe` because no `MemoChatQml` process was running.

Manual verification still needed:

- Launch MemoChatQml, log in, open Settings/More, click `切换账号` once. Expected: chat window hides and login window appears on the first click.
- Repeat for `退出登录`.
