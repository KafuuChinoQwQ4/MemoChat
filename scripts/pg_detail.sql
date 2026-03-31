\x on
SELECT * FROM pg_stat_activity WHERE datname = 'memo_pg' ORDER BY state, pid;
