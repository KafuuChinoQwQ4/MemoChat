# Plan

Summary: polish the R18 UI to match the cleaner Venera screenshots and fix blurry low-contrast text.

Steps:

- [x] Restore the R18 work from the cleanup stash without restoring the unrelated broad AI/Agent changes.
- [x] Change the R18 color system from pink translucent glass to white, pale blue, and dark text.
- [x] Replace the wide source-heavy sidebar with four fixed primary navigation buttons.
- [x] Move search input into the search page so the narrow sidebar stays simple.
- [x] Adjust buttons, cards, reader chrome, source import fields, loading text, and error banner for readable contrast.
- [x] Verify with `qmllint`, `git diff --check`, and client build.

Outcome:

- R18 now presents four primary pages in the sidebar: `书架`, `搜索`, `历史`, `漫画源`.
- Detail and reader remain reachable from comic cards/chapters.
- Direct `jm.js` source import remains in the `漫画源` page as the primary source flow.
