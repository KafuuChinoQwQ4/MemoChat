-- gateserver split Phase 2 (slice 2a) ROLLBACK: drop the per-service databases
-- and roles created by 008_db_split_media_moments_call. Run connected to the
-- maintenance database (memo_pg) by migrate_phase2_db_split.sh --rollback.
--
-- This is destructive to the NEW databases only (memo_media/memo_moments/
-- memo_call); the original memo_pg.memo tables are never dropped by the forward
-- migration, so rollback simply discards the split copies. CREATE/DROP DATABASE
-- cannot run in a transaction, so the shell driver issues the DROP DATABASE
-- statements; this file drops the roles after the databases are gone.

DO $$
BEGIN
    IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_media_app') THEN
        DROP ROLE memo_media_app;
    END IF;
    IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_moments_app') THEN
        DROP ROLE memo_moments_app;
    END IF;
    IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_call_app') THEN
        DROP ROLE memo_call_app;
    END IF;
END
$$;
