import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
TELEMETRY_UTILS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/core/network/TelemetryUtils.cpp"
MEDIA_UPLOAD_SERVICE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared/media/MediaUploadService.cpp"
MEDIA_UPLOAD_HELPERS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared/media/MediaUploadServiceHelpers.cpp"
MEDIA_UPLOAD_NETWORK = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared/media/MediaUploadServiceNetwork.cpp"
MEDIA_UPLOAD_UPLOADS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared/media/MediaUploadServiceUploads.cpp"


class AvatarUploadThreadContractTests(unittest.TestCase):
    def test_export_zipkin_span_queues_network_post_on_qapp_thread(self):
        source = TELEMETRY_UTILS.read_text(encoding="utf-8")

        self.assertRegex(source, r"QMetaObject::invokeMethod\s*\(\s*qApp\s*,")
        self.assertIn("QThread::currentThread()", source)
        self.assertIn("Qt::QueuedConnection", source)

    def test_media_upload_uses_media_base_url_and_normalizes_response_payload(self):
        source = "\n".join(
            path.read_text(encoding="utf-8")
            for path in (
                MEDIA_UPLOAD_SERVICE,
                MEDIA_UPLOAD_HELPERS,
                MEDIA_UPLOAD_NETWORK,
                MEDIA_UPLOAD_UPLOADS,
            )
        )

        self.assertIn("mediaUploadBaseUrl()", source)
        self.assertIn('mediaType.compare(QStringLiteral("avatar"), Qt::CaseInsensitive) == 0', source)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media"))', source)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media_init"))', source)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media_status"))', source)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media_chunk"))', source)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media_complete"))', source)
        self.assertIn("normalizeMediaResponse", source)


if __name__ == "__main__":
    unittest.main()
