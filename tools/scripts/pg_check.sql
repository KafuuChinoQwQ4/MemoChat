SELECT count(*) as total_conns,
       count(case when state='active' then 1 end) as active_conns,
       count(case when state='idle' then 1 end) as idle_conns
FROM pg_stat_activity
WHERE datname = 'memo_pg';
