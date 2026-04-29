Summary: polish AI conversation streaming status and thinking display behavior.

Approach:
1. Reserve vertical space for the floating generation status badge.
2. Add delegate-local collapsed/expanded state for thinking content.
3. Show thinking content while streaming, auto-collapse after completion, and allow manual expand/collapse.
4. Build client/full target and record results.

Status:
- [x] Context gathered
- [x] Implement status badge spacing
- [x] Implement collapsible thinking panel
- [x] Verify build
- [x] Review diff

Assessed: yes
