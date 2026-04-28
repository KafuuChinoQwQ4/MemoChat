SELECT name, setting, unit FROM pg_settings WHERE name IN ('max_connections', 'shared_buffers', 'superuser_reserved_connections');
