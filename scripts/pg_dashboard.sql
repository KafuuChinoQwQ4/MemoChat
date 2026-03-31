SHOW max_connections;

SELECT count(*) as total_conns,
       datname,
       count(case when state='active' then 1 end) as active_conns,
       count(case when state='idle' then 1 end) as idle_conns
FROM pg_stat_activity
GROUP BY datname
ORDER BY total_conns DESC
LIMIT 10;

SELECT count(*) as total_system_conns
FROM pg_stat_activity;
