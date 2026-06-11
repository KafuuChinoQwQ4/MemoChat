import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
QML_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
MEDIA_DIR = QML_DIR / "shared/media"
APP_DIR = QML_DIR / "app"
APP_CONTROLLER_DIR = APP_DIR / "controller"
APP_COORDINATORS = APP_DIR / "coordinators/AppCoordinators.h"
APP_CONTROLLER_HEADER = APP_CONTROLLER_DIR / "AppController.h"
MEDIA_COORDINATOR = APP_DIR / "coordinators/MediaCoordinator.cpp"
MEDIA_RUNNER = APP_DIR / "coordinators/MediaPendingAttachmentRunner.cpp"
MEDIA_RUNNER_HEADER = APP_DIR / "coordinators/MediaPendingAttachmentRunner.h"
CLIENT_CMAKE = QML_DIR / "CMakeLists.txt"
CLIENT_CMAKE_MANIFESTS = (
    QML_DIR / "app/sources.cmake",
    QML_DIR / "features/sources.cmake",
    QML_DIR / "features/auth/sources.cmake",
    QML_DIR / "features/call/sources.cmake",
    QML_DIR / "features/chat/sources.cmake",
    QML_DIR / "features/contact/sources.cmake",
    QML_DIR / "features/moments/sources.cmake",
    QML_DIR / "features/pet/sources.cmake",
    QML_DIR / "features/profile/sources.cmake",
    QML_DIR / "features/r18/sources.cmake",
    QML_DIR / "shared/sources.cmake",
    QML_DIR / "live2d/sources.cmake",
    QML_DIR / "resources/resources.cmake",
)


def client_cmake_text() -> str:
    return "\n".join(
        path.read_text(encoding="utf-8") for path in (CLIENT_CMAKE, *CLIENT_CMAKE_MANIFESTS) if path.exists()
    )


def app_controller_sources_text() -> str:
    controller_sources = "\n".join(
        path.read_text(encoding="utf-8")
        for pattern in ("AppController*.cpp", "AppChat*Binding.cpp")
        for path in sorted(APP_CONTROLLER_DIR.glob(pattern))
    )
    port_binders = "\n".join(
        path.read_text(encoding="utf-8") for path in sorted((APP_DIR / "composition").glob("*PortBinder.cpp"))
    )
    return controller_sources + "\n" + port_binders


def extract_function(source: str, signature: str) -> str:
    start = source.index(signature)
    body_start = source.index("{", start)
    depth = 0
    for index in range(body_start, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"Could not extract function body for {signature}")


class MediaUploadBoundaryTests(unittest.TestCase):
    def test_request_and_result_types_are_registered(self):
        request_header = MEDIA_DIR / "MediaUploadRequest.h"
        result_header = MEDIA_DIR / "MediaUploadResult.h"
        service_header = MEDIA_DIR / "MediaUploadService.h"
        cmake = client_cmake_text()
        shared_manifest = (QML_DIR / "shared/sources.cmake").read_text(encoding="utf-8")

        self.assertTrue(request_header.exists())
        self.assertTrue(result_header.exists())
        self.assertIn("shared/media/MediaUploadRequest.h", cmake)
        self.assertIn("shared/media/MediaUploadResult.h", cmake)
        self.assertIn("shared/media/MediaUploadRequest.h", shared_manifest)
        self.assertIn("shared/media/MediaUploadResult.h", shared_manifest)

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

    def test_pending_attachment_runner_uses_request_result_and_keeps_progress_callback(self):
        adapter = (APP_DIR / "controller/AppControllerMediaUploadQueue.cpp").read_text(encoding="utf-8")
        runner = MEDIA_RUNNER.read_text(encoding="utf-8")
        runner_header = MEDIA_RUNNER_HEADER.read_text(encoding="utf-8")
        service = (QML_DIR / "features/chat/services/UploadedAttachmentDispatchService.cpp").read_text(encoding="utf-8")
        service_header = (QML_DIR / "features/chat/services/UploadedAttachmentDispatchService.h").read_text(
            encoding="utf-8"
        )
        feature_manifest = (QML_DIR / "features/chat/sources.cmake").read_text(encoding="utf-8")

        self.assertIn("class MediaPendingAttachmentRunner", runner_header)
        self.assertIn("MediaPendingAttachmentRunnerPort", runner_header)
        self.assertIn("MediaUploadRequest request", runner)
        self.assertRegex(
            runner,
            r"(?:return\s+port\.uploadLocalFile|uploadResult\s*=\s*port\.uploadLocalFile)\s*\(\s*request\s*,",
        )
        self.assertIn("fallbackName", runner)
        self.assertIn("UploadProgressCallback", runner_header)
        self.assertIn("(int percent, const QString& stage)", runner)
        self.assertIn("Qt::QueuedConnection", runner)
        self.assertIn("QFutureWatcher<MediaUploadResult>", runner)
        self.assertIn("QtConcurrent::run", runner)
        self.assertIn("dispatchUploadedAttachment", runner + runner_header)
        self.assertIn("advanceQueue", runner + runner_header)
        self.assertIn("resetQueue", runner + runner_header)
        self.assertIn(
            "_features.chatFeatureController.dispatchUploadedAttachment(attachment, uploaded, destination)", adapter
        )
        self.assertIn("MediaPendingAttachmentRunnerPort{", adapter)
        self.assertNotIn("processPendingAttachmentQueue", adapter + runner + runner_header)
        self.assertNotIn("QFutureWatcher<MediaUploadResult>", adapter)
        self.assertNotIn("QtConcurrent::run", adapter)
        self.assertNotIn("Qt::QueuedConnection", adapter)
        self.assertNotIn("bool AppController::sendUploadedAttachmentToDialog", adapter + runner)
        self.assertNotIn("MessageContentCodec::encodeImage", adapter + runner)
        self.assertNotIn("MessageContentCodec::encodeFile", adapter + runner)
        self.assertNotIn("QJsonDocument(payloadObj)", adapter + runner)
        self.assertNotIn("ID_GROUP_CHAT_MSG_REQ", adapter)
        self.assertNotIn("UploadedAttachmentDispatchPort", adapter)
        self.assertNotIn("dispatchPrivatePacket", adapter)
        self.assertNotIn("dispatchGroupPayload", adapter)
        self.assertNotIn("slot_send_data", adapter)

        self.assertIn("class UploadedAttachmentDispatchService", service_header)
        self.assertIn("UploadedAttachmentDispatchPort", service_header)
        self.assertIn("UploadedAttachmentDestination", service_header)
        self.assertIn("std::function<void(int, const QByteArray&)> dispatchGroupPayload;", service_header)
        self.assertIn("MessageContentCodec::encodeImage", service)
        self.assertIn("MessageContentCodec::encodeFile", service)
        self.assertIn("OutgoingChatPacket packet", service)
        self.assertIn("buildGroupPayload", service)
        self.assertIn("constexpr int kGroupChatRequestId = 1044;", service)
        self.assertNotIn("AppController", service + service_header)
        self.assertIn("features/chat/services/UploadedAttachmentDispatchService.cpp", feature_manifest)
        self.assertIn("features/chat/services/UploadedAttachmentDispatchService.h", feature_manifest)

    def test_media_coordinator_uses_narrow_send_port_not_appcontroller_friend_access(self):
        app_header = APP_CONTROLLER_HEADER.read_text(encoding="utf-8")
        coordinators = APP_COORDINATORS.read_text(encoding="utf-8")
        media = MEDIA_COORDINATOR.read_text(encoding="utf-8")
        media_class = coordinators[
            coordinators.index("class MediaCoordinator") : coordinators.index(
                "\n};", coordinators.index("class MediaCoordinator")
            )
            + 3
        ]

        self.assertNotIn("friend class MediaCoordinator", app_header)
        self.assertIn("struct MediaSendPort", coordinators)
        self.assertIn("std::function<MediaSendSnapshot()> snapshot", coordinators)
        for callback in (
            "dispatchPrivateContent",
            "dispatchGroupContent",
            "beginPendingAttachmentSend",
            "startPendingAttachmentRunner",
            "addPendingAttachments",
            "removePendingAttachmentById",
            "clearPendingAttachmentsForDialog",
            "setCurrentPendingAttachments",
        ):
            with self.subTest(callback=callback):
                self.assertIn(callback, coordinators)
        self.assertNotIn("processPendingAttachmentQueue", coordinators)

        self.assertIn("explicit MediaCoordinator(MediaSendPort port)", media_class)
        self.assertIn("MediaSendPort _port", media_class)
        self.assertNotIn("AppController&", media_class)
        self.assertNotIn("_app", media_class)
        self.assertNotIn('#include "AppController.h"', media)
        self.assertIn("MediaCoordinator::MediaCoordinator(MediaSendPort port)", media)
        self.assertNotIn("MediaCoordinator::MediaCoordinator(AppController&", media)
        self.assertNotRegex(media, r"\b_app\b")
        self.assertNotRegex(media, r"\bAppController\b")

    def test_appcontroller_assembles_media_send_port_instead_of_passing_self(self):
        sources = app_controller_sources_text()
        media_binding = extract_function(sources, "void AppController::bindChatFeatureMediaPorts()")

        self.assertNotIn("std::make_unique<MediaCoordinator>(*this)", sources)
        self.assertNotIn("new MediaCoordinator(*this)", sources)
        self.assertIn("MediaSendPort", sources)
        self.assertIn("MediaSendSnapshot", sources)
        self.assertIn("std::make_unique<MediaCoordinator>", sources)

        for removed_wrapper in (
            r"\bvoid\s+AppController::sendCurrentComposerPayload\s*\(",
            r"\bvoid\s+AppController::sendImageMessage\s*\(",
            r"\bvoid\s+AppController::sendFileMessage\s*\(",
            r"\bvoid\s+AppController::removePendingAttachment\s*\(",
            r"\bvoid\s+AppController::clearPendingAttachments\s*\(",
        ):
            with self.subTest(removed_wrapper=removed_wrapper):
                self.assertIsNone(re.search(removed_wrapper, sources))

        for direct_media_binding in (
            "_media_coordinator->sendCurrentComposerPayload(text)",
            "_media_coordinator->sendImageMessage()",
            "_media_coordinator->sendFileMessage()",
        ):
            with self.subTest(direct_media_binding=direct_media_binding):
                self.assertIn(direct_media_binding, media_binding)

        for direct_media_binding in (
            "_media_coordinator->removePendingAttachment(attachmentId)",
            "_media_coordinator->clearPendingAttachments()",
        ):
            with self.subTest(direct_media_binding=direct_media_binding):
                self.assertIn(direct_media_binding, media_binding)

        for stale_app_wrapper_call in (
            r"(?<!->)\bsendCurrentComposerPayload\(text\);",
            r"(?<!->)\bsendImageMessage\(\);",
            r"(?<!->)\bsendFileMessage\(\);",
        ):
            with self.subTest(stale_app_wrapper_call=stale_app_wrapper_call):
                self.assertIsNone(re.search(stale_app_wrapper_call, media_binding))

        for stale_app_wrapper_call in (
            r"(?<!->)\bremovePendingAttachment\(attachmentId\);",
            r"(?<!->)\bclearPendingAttachments\(\);",
        ):
            with self.subTest(stale_app_wrapper_call=stale_app_wrapper_call):
                self.assertIsNone(re.search(stale_app_wrapper_call, media_binding))

        for required_state in (
            "currentPendingAttachments()",
            "_media_upload_state.inProgress",
            "currentDialogUid()",
            "_chat_state.uid",
            "currentGroupId()",
            "_features.chatFeatureController.replyToMsgId()",
            "_features.chatFeatureController.replyTargetName()",
            "_features.chatFeatureController.replyPreviewText()",
        ):
            with self.subTest(required_state=required_state):
                self.assertIn(required_state, sources)

        for required_callback in (
            "setTip(tip, isError)",
            "_features.chatFeatureController.dispatchCurrentPrivateContent(content, preview)",
            "_features.chatFeatureController.dispatchCurrentGroupContent(content, preview)",
            "setPendingReplyContext(QString(), QString(), QString())",
            "_features.chatFeatureController.beginPendingAttachmentSend",
            "startPendingAttachmentRunner()",
            "addPendingAttachments(attachments)",
            "removePendingAttachmentById(attachmentId)",
            "_features.chatFeatureController.clearPendingAttachmentsForDialog",
            "setCurrentPendingAttachments(attachments)",
        ):
            with self.subTest(required_callback=required_callback):
                self.assertIn(required_callback, sources)
        self.assertNotIn("processPendingAttachmentQueue", sources)

    def test_media_send_behaviour_preserves_composer_picker_and_guard_paths(self):
        media = MEDIA_COORDINATOR.read_text(encoding="utf-8")
        send_text = extract_function(media, "void MediaCoordinator::sendTextMessage(const QString& text)")
        send_composer = extract_function(
            media, "void MediaCoordinator::sendCurrentComposerPayload(const QString& text)"
        )
        send_image = extract_function(media, "void MediaCoordinator::sendImageMessage()")
        send_file = extract_function(media, "void MediaCoordinator::sendFileMessage()")
        remove_attachment = extract_function(
            media, "void MediaCoordinator::removePendingAttachment(const QString& attachmentId)"
        )
        clear_attachments = extract_function(media, "void MediaCoordinator::clearPendingAttachments()")

        self.assertIn("_port.snapshot()", send_composer)
        self.assertIn("snapshot.uploadInProgress", send_composer)
        self.assertIn("已有文件上传中，请稍后", send_composer)
        self.assertIn("snapshot.pendingAttachments", send_composer)
        self.assertIn("_port.beginPendingAttachmentSend", send_composer)
        self.assertIn("_port.startPendingAttachmentRunner", send_composer)
        self.assertNotIn("_port.processPendingAttachmentQueue", send_composer)
        self.assertIn("sendTextMessage(text)", send_composer)

        self.assertIn("trimmed().isEmpty()", send_text)
        self.assertIn("content.size() > 1024", send_text)
        self.assertIn("MessageContentCodec::encodeReplyText", send_text)
        self.assertIn("_port.dispatchGroupContent", send_text)
        self.assertIn("_port.dispatchPrivateContent", send_text)
        self.assertIn("_port.cancelReply", send_text)

        self.assertIn("LocalFilePickerService::pickImageUrls", send_image)
        self.assertIn("_port.addPendingAttachments", send_image)
        self.assertIn("LocalFilePickerService::pickFileUrls", send_file)
        self.assertIn("_port.addPendingAttachments", send_file)

        for guarded_function in (remove_attachment, clear_attachments):
            with self.subTest(guarded_function=guarded_function.splitlines()[0]):
                self.assertIn("_port.snapshot()", guarded_function)
                self.assertRegex(guarded_function, r"(snapshot|_port\.snapshot\(\))\.uploadInProgress")

        self.assertIn("_port.removePendingAttachmentById", remove_attachment)
        self.assertIn("snapshot.currentDialogUid", clear_attachments)
        self.assertIn("_port.clearPendingAttachmentsForDialog", clear_attachments)
        self.assertIn("_port.setCurrentPendingAttachments(QVariantList())", clear_attachments)


if __name__ == "__main__":
    unittest.main()
