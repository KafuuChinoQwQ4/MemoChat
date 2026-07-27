from pathlib import Path

import yaml

from tests.python.support.paths import repo_root

OTEL_CONFIG = repo_root() / "infra/deploy/local/observability/otel/config.yaml"


def test_filelog_receiver_cannot_ingest_runtime_credentials_or_business_data() -> None:
    config = yaml.safe_load(OTEL_CONFIG.read_text(encoding="utf-8"))
    receiver = config["receivers"]["filelog"]
    included = receiver["include"]
    excluded = receiver["exclude"]

    assert included
    assert all("/logs/" in pattern for pattern in included)
    assert not any(pattern.endswith("/**/*.json") and "/logs/" not in pattern for pattern in included)
    assert any("/data/" in pattern for pattern in excluded)
    assert any("/credentials/" in pattern for pattern in excluded)
    assert any("/cookies/" in pattern for pattern in excluded)
    assert any("/sessions/" in pattern for pattern in excluded)
