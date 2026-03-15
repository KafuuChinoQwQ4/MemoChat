# Local Environment Rules

- When this Windows machine has a `D:` drive, prefer storing large local-only artifacts on `D:` instead of `C:`.
- This preference applies to Docker data, build directories, package caches, downloaded archives, temporary runtime data, load-test outputs, and other bulky non-source files.
- Keep source-controlled project files in the repository as usual. Only redirect local-generated or machine-specific heavy data to `D:`.
- If a tool defaults to `C:` and supports relocation, choose a `D:` path first unless the user explicitly requests otherwise.
- If an existing `C:` location is already in use and is safe to migrate, prefer migrating it to `D:` rather than growing usage on `C:`.
