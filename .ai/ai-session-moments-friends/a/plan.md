Summary: make AI session deletion reachable and add friend-filtered Moments navigation.

Approach:
1. Fix AI session delegate layering and add a confirmation dialog before delete.
2. Replace empty Moments side panel with all-feed plus friend list entries from contactModel.
3. Add optional author_uid filtering to MomentsController and server list endpoints.
4. Pass selected friend state through ChatShellPage into the left panel and feed header.
5. Verify with diff review and cmake --build --preset msvc2022-full.

Status:
- [x] Context gathered
- [x] Plan assessed against existing symbols
- [x] Implement UI changes
- [x] Implement client/server filtering
- [x] Build verification
- [x] Review diff

Assessed: yes
