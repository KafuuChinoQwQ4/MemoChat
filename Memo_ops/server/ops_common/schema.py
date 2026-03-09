from __future__ import annotations

from typing import Dict

import pymysql


SCHEMA_SQL = [
    """
    CREATE TABLE IF NOT EXISTS ops_source (
        source_id BIGINT AUTO_INCREMENT PRIMARY KEY,
        source_type VARCHAR(32) NOT NULL,
        source_path VARCHAR(512) NOT NULL,
        source_hash VARCHAR(128) NOT NULL DEFAULT '',
        first_seen_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        last_seen_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        metadata_json JSON NULL,
        UNIQUE KEY uk_source_type_path (source_type, source_path)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_ingest_cursor (
        cursor_key VARCHAR(255) PRIMARY KEY,
        cursor_value JSON NULL,
        updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_loadtest_run (
        run_id VARCHAR(64) PRIMARY KEY,
        suite_name VARCHAR(255) NOT NULL DEFAULT '',
        scenario_name VARCHAR(64) NOT NULL DEFAULT '',
        test_name VARCHAR(128) NOT NULL DEFAULT '',
        file_path VARCHAR(512) NOT NULL,
        report_hash VARCHAR(128) NOT NULL DEFAULT '',
        started_at DATETIME NULL,
        finished_at DATETIME NULL,
        status VARCHAR(32) NOT NULL DEFAULT 'unknown',
        success_count INT NOT NULL DEFAULT 0,
        failure_count INT NOT NULL DEFAULT 0,
        summary_json JSON NULL,
        top_errors_json JSON NULL,
        phase_breakdown_json JSON NULL,
        preconditions_json JSON NULL,
        data_mutation_json JSON NULL,
        imported_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        UNIQUE KEY uk_report_path_hash (file_path, report_hash)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_loadtest_case (
        case_id BIGINT AUTO_INCREMENT PRIMARY KEY,
        run_id VARCHAR(64) NOT NULL,
        case_name VARCHAR(255) NOT NULL DEFAULT '',
        status VARCHAR(32) NOT NULL DEFAULT '',
        success_count INT NOT NULL DEFAULT 0,
        failure_count INT NOT NULL DEFAULT 0,
        p50_ms DOUBLE NOT NULL DEFAULT 0,
        p95_ms DOUBLE NOT NULL DEFAULT 0,
        avg_ms DOUBLE NOT NULL DEFAULT 0,
        extras_json JSON NULL,
        created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        UNIQUE KEY uk_run_sample (run_id, case_name),
        KEY idx_run_id (run_id)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_loadtest_error_bucket (
        bucket_id BIGINT AUTO_INCREMENT PRIMARY KEY,
        run_id VARCHAR(64) NOT NULL,
        error_key VARCHAR(255) NOT NULL,
        error_count INT NOT NULL DEFAULT 0,
        sample_message VARCHAR(1024) NOT NULL DEFAULT '',
        created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        UNIQUE KEY uk_run_error (run_id, error_key),
        KEY idx_error_key (error_key)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_service_snapshot (
        snapshot_id BIGINT AUTO_INCREMENT PRIMARY KEY,
        service_name VARCHAR(64) NOT NULL,
        instance_name VARCHAR(128) NOT NULL DEFAULT '',
        observed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        process_count INT NOT NULL DEFAULT 0,
        status VARCHAR(32) NOT NULL DEFAULT 'unknown',
        cpu_percent DOUBLE NOT NULL DEFAULT 0,
        memory_rss_bytes BIGINT NOT NULL DEFAULT 0,
        online_users INT NOT NULL DEFAULT 0,
        error_rate DOUBLE NOT NULL DEFAULT 0,
        latency_p95_ms DOUBLE NOT NULL DEFAULT 0,
        qps DOUBLE NOT NULL DEFAULT 0,
        port_up TINYINT(1) NOT NULL DEFAULT 0,
        details_json JSON NULL,
        KEY idx_service_snapshot (service_name, observed_at),
        KEY idx_instance_snapshot (instance_name, observed_at)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_service_metric_minute (
        metric_id BIGINT AUTO_INCREMENT PRIMARY KEY,
        service_name VARCHAR(64) NOT NULL,
        instance_name VARCHAR(128) NOT NULL DEFAULT '',
        minute_utc DATETIME NOT NULL,
        cpu_percent_avg DOUBLE NOT NULL DEFAULT 0,
        memory_rss_bytes_avg DOUBLE NOT NULL DEFAULT 0,
        online_users_avg DOUBLE NOT NULL DEFAULT 0,
        qps_avg DOUBLE NOT NULL DEFAULT 0,
        error_rate_avg DOUBLE NOT NULL DEFAULT 0,
        latency_p95_ms_avg DOUBLE NOT NULL DEFAULT 0,
        samples INT NOT NULL DEFAULT 0,
        UNIQUE KEY uk_metric_minute (service_name, instance_name, minute_utc),
        KEY idx_metric_name_ts (service_name, minute_utc)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_log_stream (
        stream_id BIGINT AUTO_INCREMENT PRIMARY KEY,
        service_name VARCHAR(64) NOT NULL,
        service_instance VARCHAR(128) NOT NULL DEFAULT '',
        source_path VARCHAR(512) NOT NULL,
        source_type VARCHAR(32) NOT NULL DEFAULT 'json_log',
        updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
        UNIQUE KEY uk_stream_path (source_path),
        KEY idx_stream_service (service_name, service_instance)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_log_event_index (
        id BIGINT AUTO_INCREMENT PRIMARY KEY,
        service_name VARCHAR(64) NOT NULL,
        service_instance VARCHAR(128) NOT NULL DEFAULT '',
        level VARCHAR(16) NOT NULL DEFAULT '',
        event_name VARCHAR(128) NOT NULL DEFAULT '',
        trace_id VARCHAR(128) NOT NULL DEFAULT '',
        request_id VARCHAR(128) NOT NULL DEFAULT '',
        uid VARCHAR(64) NOT NULL DEFAULT '',
        ts_utc DATETIME NOT NULL,
        duration_ms DOUBLE NOT NULL DEFAULT 0,
        file_path VARCHAR(512) NOT NULL,
        line_number BIGINT NOT NULL,
        message VARCHAR(1024) NOT NULL DEFAULT '',
        attrs_json JSON NULL,
        raw_json TEXT NULL,
        UNIQUE KEY uk_log_line (file_path, line_number),
        KEY idx_service_ts (service_name, ts_utc),
        KEY idx_trace_id (trace_id),
        KEY idx_level_ts (level, ts_utc)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_trace_index (
        trace_id VARCHAR(128) PRIMARY KEY,
        first_seen_at DATETIME NOT NULL,
        last_seen_at DATETIME NOT NULL,
        services_json JSON NULL,
        log_count INT NOT NULL DEFAULT 0,
        span_count INT NOT NULL DEFAULT 0,
        root_service VARCHAR(64) NOT NULL DEFAULT '',
        status VARCHAR(32) NOT NULL DEFAULT 'active',
        updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_alert_rule (
        alert_rule_id BIGINT AUTO_INCREMENT PRIMARY KEY,
        rule_name VARCHAR(128) NOT NULL,
        rule_type VARCHAR(64) NOT NULL DEFAULT '',
        enabled TINYINT(1) NOT NULL DEFAULT 1,
        threshold_json JSON NULL,
        updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
        UNIQUE KEY uk_rule_name (rule_name)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE IF NOT EXISTS ops_platform_setting (
        setting_key VARCHAR(128) PRIMARY KEY,
        setting_value_json JSON NULL,
        updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
]


DEFAULT_RULES = [
    ("missing-service", "service_presence", '{"missing_service_after_sec": 30}'),
    ("high-error-rate", "error_rate", '{"error_rate_warn": 0.25}'),
    ("high-cpu", "cpu", '{"cpu_warn": 85.0}'),
]


INDEX_SQL = [
    "ALTER TABLE ops_log_event_index ADD KEY idx_service_instance_ts (service_name, service_instance, ts_utc)",
    "ALTER TABLE ops_log_event_index ADD KEY idx_request_id_ts (request_id, ts_utc)",
    "ALTER TABLE ops_log_event_index ADD KEY idx_event_name_ts (event_name, ts_utc)",
]


def init_schema(mysql_cfg: Dict[str, object]) -> None:
    root = pymysql.connect(
        host=str(mysql_cfg["host"]),
        port=int(mysql_cfg["port"]),
        user=str(mysql_cfg["user"]),
        password=str(mysql_cfg["password"]),
        charset=str(mysql_cfg.get("charset", "utf8mb4")),
        autocommit=True,
    )
    schema = str(mysql_cfg["schema"])
    with root.cursor() as cur:
        cur.execute(f"CREATE DATABASE IF NOT EXISTS `{schema}` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
    root.close()

    conn = pymysql.connect(
        host=str(mysql_cfg["host"]),
        port=int(mysql_cfg["port"]),
        user=str(mysql_cfg["user"]),
        password=str(mysql_cfg["password"]),
        database=schema,
        charset=str(mysql_cfg.get("charset", "utf8mb4")),
        autocommit=True,
    )
    with conn.cursor() as cur:
        for sql in SCHEMA_SQL:
            cur.execute(sql)
        for sql in INDEX_SQL:
            try:
                cur.execute(sql)
            except pymysql.err.OperationalError as exc:
                if exc.args and int(exc.args[0]) in {1061, 1060, 1091}:
                    pass
                else:
                    raise
        for name, rule_type, threshold_json in DEFAULT_RULES:
            cur.execute(
                """
                INSERT INTO ops_alert_rule(rule_name, rule_type, enabled, threshold_json)
                VALUES(%s, %s, 1, CAST(%s AS JSON))
                ON DUPLICATE KEY UPDATE rule_type = VALUES(rule_type), threshold_json = VALUES(threshold_json)
                """,
                (name, rule_type, threshold_json),
            )
    conn.close()
