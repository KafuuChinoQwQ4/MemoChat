import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
QML_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
MEDIA_DIR = QML_DIR / "shared/media"
APP_DIR = QML_DIR / "app"


class MediaUploadBoundaryTests(unittest.TestCase):
    def test_request_and_result_types_are_registered(self):
        request_header = MEDIA_DIR / "MediaUploadRequest.h"
        result_header = MEDIA_DIR / "MediaUploadResult.h"
        service_header = MEDIA_DIR / "MediaUploadService.h"
        cmake = (QML_DIR / "CMakeLists.txt").read_text(encoding="utf-8")

        self.assertTrue(request_header.exists())
        self.assertTrue(result_header.exists())
        self.assertIn("shared/media/MediaUploadRequest.h", cmake)
        self.assertIn("shared/media/MediaUploadResult.h", cmake)

        request = request_header.read_text(encoding="utf-8")
        result = result_header.read_text(encoding="utf-8")
        service = service_header.read_text(encoding="utf-8")

        for field in ("localFileUrl", "mediaType", "uid", "token", "fallbackName"):
            self.assertRegex(request, rf"\bQString\b.*\b{field}\b|\bint\b.*\b{field}\b")

        self.assertIn("UploadedMediaInfo info", result)
        self.assertIn("QString errorText", result)
        self.assertIn("bool ok", result)
        self.assertIn("MediaUploadResult uploadLocalFile(const MediaUploadRequest& request", service)
        self.assertIn("bool uploadLocalFile(const QString& localFileUrl", service)

    def test_avatar_and_chunked_routes_are_preserved(self):
        service = (MEDIA_DIR / "MediaUploadService.cpp").read_text(encoding="utf-8")
        uploads = (MEDIA_DIR / "MediaUploadServiceUploads.cpp").read_text(encoding="utf-8")
        combined = service + "\n" + uploads

        self.assertIn('mediaType.compare(QStringLiteral("avatar"), Qt::CaseInsensitive) == 0', combined)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media"))', uploads)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media_init"))', uploads)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media_status"))', uploads)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media_chunk"))', uploads)
        self.assertIn('mediaUploadUrl(QStringLiteral("/upload_media_complete"))', uploads)

    def test_old_upload_signature_wraps_request_result_boundary(self):
        service = (MEDIA_DIR / "MediaUploadService.cpp").read_text(encoding="utf-8")

        self.assertIn("MediaUploadRequest request", service)
        self.assertIn("MediaUploadResult result = uploadLocalFile(request, progress)", service)
        self.assertIn("result.info.fileName = request.fallbackName.trimmed();", service)
        self.assertRegex(
            service,
            r"bool\s+MediaUploadService::uploadLocalFile\([^)]*UploadedMediaInfo\*\s*outInfo[^)]*QString\*\s*errorText",
            re.S,
        )

    def test_attachment_queue_uses_request_result_and_keeps_progress_callback(self):
        queue = (APP_DIR / "AppControllerMediaUploadQueue.cpp").read_text(encoding="utf-8")

        self.assertIn("MediaUploadRequest request", queue)
        self.assertRegex(
            queue,
            r"MediaUploadResult\s+result\s*=\s*MediaUploadService::uploadLocalFile\s*\(\s*request\s*,",
        )
        self.assertIn("fallbackName", queue)
        self.assertIn("[this, attachmentIndex](int percent, const QString& stage)", queue)
        self.assertIn("Qt::QueuedConnection", queue)


if __name__ == "__main__":
    unittest.main()
