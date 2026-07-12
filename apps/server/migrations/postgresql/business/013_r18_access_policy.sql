CREATE SCHEMA IF NOT EXISTS memo;

ALTER TABLE IF EXISTS memo."user"
    ADD COLUMN IF NOT EXISTS adult_attested_at_ms bigint NOT NULL DEFAULT 0,
    ADD COLUMN IF NOT EXISTS r18_access_state smallint NOT NULL DEFAULT 0;

DO $$
DECLARE
    user_table regclass := to_regclass('memo."user"');
BEGIN
    IF user_table IS NOT NULL THEN
        IF NOT EXISTS (
            SELECT 1
            FROM pg_constraint
            WHERE conname = 'ck_user_r18_access_state'
              AND conrelid = user_table
        ) THEN
            ALTER TABLE memo."user"
                ADD CONSTRAINT ck_user_r18_access_state
                CHECK (r18_access_state IN (0, 1, 2));
        END IF;
        IF NOT EXISTS (
            SELECT 1
            FROM pg_constraint
            WHERE conname = 'ck_user_adult_attested_at_ms'
              AND conrelid = user_table
        ) THEN
            ALTER TABLE memo."user"
                ADD CONSTRAINT ck_user_adult_attested_at_ms
                CHECK (adult_attested_at_ms >= 0);
        END IF;
    END IF;
END
$$;
