import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
TELEMETRY_UTILS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/core/network/TelemetryUtils.cpp"
MEDIA_UPLOAD_SERVICE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared/media/MediaUploadService.cpp"


class AvatarUploadThreadContractTests(unittest.TestCase):
    def test_export_zipkin_span_queues_network_post_on_qapp_thread(self):
        source = TELEMETRY_UTILS.read_text(encoding="utf-8")

        self.assertIn("QMetaObject::invokeMethod(qApp", source)
        self.assertIn("QThread::currentThread()", source)
        self.assertIn("Qt::QueuedConnection", source)

    def test_media_upload_uses_media_base_url_and_normalizes_response_payload(self):
        source = MEDIA_UPLOAD_SERVICE.read_text(encoding="utf-8")

        self.assertIn("mediaUploadBaseUrl()", source)
        self.assertIn('mediaType.compare(QStringLiteral("avatar"), Qt::CaseInsensitive) == 0', source)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media"))', source)
        self.assertIn("mediaUploadUrl(QStringLiteral(\"/upload_media_init\"))", source)
        self.assertIn("mediaUploadUrl(QStringLiteral(\"/upload_media_status\"))", source)
        self.assertIn("mediaUploadUrl(QStringLiteral(\"/upload_media_chunk\"))", source)
        self.assertIn("mediaUploadUrl(QStringLiteral(\"/upload_media_complete\"))", source)
        self.assertIn("normalizeMediaResponse", source)


if __name__ == "__main__":
    unittest.main()
