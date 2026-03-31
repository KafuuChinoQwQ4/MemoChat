SELECT datname, count(*) FROM pg_stat_activity GROUP BY datname ORDER BY count(*) DESC;
