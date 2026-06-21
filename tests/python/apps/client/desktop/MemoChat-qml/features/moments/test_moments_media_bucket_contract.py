import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
SERVER_CORE = REPO_ROOT / "apps/server/core"


def read(path):
    return path.read_text(encoding="utf-8-sig")


class MomentsMediaBucketContractTests(unittest.TestCase):
    def test_moments_upload_uses_dedicated_media_types_but_keeps_item_types(self):
        source = read(CLIENT / "features/moments/parsing/MomentsPublishPayload.cpp")

        self.assertIn("momentUploadMediaType", source)
        self.assertIn('QStringLiteral("moment_image")', source)
        self.assertIn('QStringLiteral("moment_video")', source)
        self.assertIn("const QString uploadMediaType = momentUploadMediaType(attachmentType);", source)
        self.assertIn("uploadMediaType,", source)

        self.assertIn('item["media_type"] = QStringLiteral("image");', source)
        self.assertIn('item["media_type"] = QStringLiteral("video");', source)
        self.assertIn('if (attachmentType == QStringLiteral("video"))', source)

    def test_moments_media_download_urls_use_authenticated_controller_helper(self):
        header = read(CLIENT / "features/moments/controller/MomentsController.h")
        controller = read(CLIENT / "features/moments/controller/MomentsController.cpp")
        feed = read(CLIENT / "features/moments/view/MomentsFeedPane.qml")
        delegate = read(CLIENT / "features/moments/view/MomentsDelegate.qml")
        detail = read(CLIENT / "features/moments/view/MomentsDetailPopup.qml")
        preview = read(CLIENT / "features/moments/view/MomentsImagePreviewPopup.qml")

        self.assertIn("Q_INVOKABLE QString mediaUrlForKey(const QString& mediaKey) const;", header)
        self.assertIn('#include "IconPathUtils.h"', controller)
        self.assertIn("mediaKeyDownloadUrl(trimmed)", controller)
        self.assertIn("controller: root.momentsController", feed)
        self.assertIn('root.controller ? root.controller.mediaUrlForKey(key) : ""', delegate)
        self.assertIn('root.controller ? root.controller.mediaUrlForKey(key) : ""', detail)
        self.assertIn("property var mediaUrlResolver", preview)

        moments_qml_sources = [delegate, detail, preview]
        for source in moments_qml_sources:
            self.assertNotIn('gateMediaUrlPrefix + "/media/download?asset="', source)
            self.assertNotIn('gatewayPrefix + "/media/download?asset="', source)

    def test_moments_publish_page_serializes_attachments_before_crossing_qml_cpp_boundary(self):
        header = read(CLIENT / "features/moments/controller/MomentsController.h")
        publish_cpp = read(CLIENT / "features/moments/controller/MomentsControllerPublish.cpp")
        publish_qml = read(CLIENT / "features/moments/view/MomentsPublishPage.qml")

        self.assertIn(
            "Q_INVOKABLE void publishDraftMomentJson(const QString& text, int visibility, const QString& attachmentsJson);",
            header,
        )
        self.assertIn("QJsonDocument::fromJson", publish_cpp)
        self.assertIn("value.toObject().toVariantMap()", publish_cpp)
        self.assertIn("function draftAttachmentObject(item)", publish_qml)
        self.assertIn("function draftAttachmentsJson()", publish_qml)
        self.assertIn("JSON.stringify(list)", publish_qml)
        self.assertIn("root.controller.publishDraftMomentJson(", publish_qml)
        self.assertNotIn("root.controller.publishDraftMoment(\n", publish_qml)

    def test_s3_storage_routes_moment_uploads_to_dedicated_bucket(self):
        source = read(SERVER_CORE / "MediaService/core/storage/S3MediaStorage.cpp")
        header = read(SERVER_CORE / "MediaService/core/storage/S3MediaStorage.h")

        self.assertIn('_bucket_moments = minio["BucketMoments"];', source)
        self.assertIn('_bucket_moments = "memochat-moments";', source)
        self.assertIn('t == "moment_image"', source)
        self.assertIn('t == "moment_video"', source)
        self.assertIn("out_bucket = _bucket_moments;", source)
        self.assertIn("first == _bucket_moments", source)
        self.assertIn("std::string _bucket_moments;", header)

    def test_gate_configs_define_moments_bucket(self):
        # GateServer monolith retired: its main config.ini/gate2.ini are gone; the moments bucket
        # config now lives in the focused MediaService gateway ini. Runtime GateServer1/2 dirs are kept.
        config_paths = [
            SERVER_CORE / "MediaService/mediagateway.ini",
            REPO_ROOT / "infra/Memo_ops/runtime/services/GateServer1/config.ini",
            REPO_ROOT / "infra/Memo_ops/runtime/services/GateServer2/config.ini",
        ]

        for path in config_paths:
            with self.subTest(path=path):
                source = read(path)
                self.assertIn("BucketMoments=memochat-moments", source)

    def test_local_startup_initializes_moments_bucket(self):
        script = read(REPO_ROOT / "tools/scripts/status/ensure_minio_buckets.sh")
        startup = read(REPO_ROOT / "tools/scripts/status/start-all-services.sh")

        self.assertIn("MINIO_BUCKET_MOMENTS:-memochat-moments", script)
        self.assertIn("docker exec memochat-minio /usr/bin/mc alias set", script)
        self.assertIn("mc mb --ignore-existing", script)
        self.assertIn('ensure_minio_buckets.sh" || echo "  [WARN] failed to ensure MinIO buckets"', startup)

    def test_moment_image_uses_image_upload_limit(self):
        client = read(CLIENT / "shared/media/MediaUploadService.cpp")
        gate_h1 = read(SERVER_CORE / "MediaService/domain/services/media/MediaService.cpp")
        gate_shared = read(SERVER_CORE / "MediaService/core/support/Http2MediaSupport.cpp")

        self.assertIn("usesImageUploadLimit", client)
        self.assertIn('normalized == QStringLiteral("moment_image")', client)
        self.assertIn('media_type == "image" || media_type == "moment_image"', gate_h1)
        self.assertIn('media_type == "image" || media_type == "moment_image"', gate_shared)


if __name__ == "__main__":
    unittest.main()
