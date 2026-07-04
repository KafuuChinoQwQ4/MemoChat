-- gateserver microservice split Phase 2 (slice 2a): per-service PostgreSQL
-- databases for the safely-splittable GateServer domains (media / moments /
-- call). Shared single PG instance, one database + one least-privilege role per
-- service. r18 has no PG tables (Redis/external); account is deferred to slice 2b
-- because ChatServer still JOINs the user table (cross-DB JOIN is impossible).
--
-- This file is the DATABASE + ROLE bootstrap. It is run connected to the
-- maintenance database (memo_pg) by migrate_phase2_db_split.sh, which then loads
-- the per-database schema files. Idempotent: guarded by IF NOT EXISTS / DO blocks.
--
-- Rollback: 008_db_split_media_moments_call_rollback.sql (drops the databases and
-- roles). Data migration + verification are orchestrated by the shell script.

-- ── Roles (least privilege; passwords are injected by migrate_phase2_db_split.sh)
DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_media_app') THEN
        CREATE ROLE memo_media_app LOGIN;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_moments_app') THEN
        CREATE ROLE memo_moments_app LOGIN;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_call_app') THEN
        CREATE ROLE memo_call_app LOGIN;
    END IF;
END
$$;

-- ── Databases (CREATE DATABASE cannot run inside a transaction/ DO block, so the
-- shell driver creates them with `CREATE DATABASE ... ` guarded by a prior
-- existence check via psql \gexec). See migrate_phase2_db_split.sh.
