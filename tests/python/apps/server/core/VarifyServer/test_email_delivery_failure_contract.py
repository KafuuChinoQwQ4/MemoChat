import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
VARIFY_DIR = REPO_ROOT / "apps/server/core/VarifyServer"
# VarifyServer is a self-contained microservice with its own error-code const.hpp (it no longer
# pulls the retired GateServer monolith's gate-core const.h; the shared value also lives in
# GateShared/core/config/const.h). The VarifyServer-local copy is this service's contract.
VARIFY_CONST = REPO_ROOT / "apps/server/core/VarifyServer/const.hpp"
CLIENT_AUTH_RESPONSES = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/session/SessionAuthCoordinatorAuthResponses.cpp"
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class EmailDeliveryFailureContractTests(unittest.TestCase):
    def test_email_sender_returns_failure_when_smtp_transaction_fails(self):
        source = read(VARIFY_DIR / "EmailSender.cpp")

        self.assertIn("smtp_ok", source)
        self.assertRegex(source, r"smtp_ok\s*=\s*smtp_ok\s*&&\s*ssl_expect_code\(235\);")
        self.assertRegex(source, r"smtp_ok\s*=\s*smtp_ok\s*&&\s*expect_code\(sock,\s*235\);")
        self.assertIsNotNone(
            re.search(
                r"if \(!smtp_ok\)\s*\{(?:(?!return true;).)*varify\.email\.send_failed(?:(?!return true;).)*return false;",
                source,
                re.S,
            ),
            "EmailSender::Send must return false after any failed SMTP transaction step",
        )

    def test_email_sender_fails_fast_without_committed_smtp_credentials(self):
        source = read(VARIFY_DIR / "EmailSender.cpp")
        config = read(VARIFY_DIR / "config.ini")
        varify2 = read(VARIFY_DIR / "varify2.ini")

        for text in (config, varify2):
            with self.subTest(config_hash=hash(text)):
                self.assertNotIn("kafu_chino", text)
                self.assertNotIn("hrkhkvgptixfdfja", text)
                self.assertIn("MEMOCHAT_EMAIL_SMTPPASS", text)
                self.assertRegex(text, r"(?m)^SMTPUser=$")
                self.assertRegex(text, r"(?m)^SMTPPass=$")

        self.assertIn("varify.email.config_missing", source)
        self.assertIn("SMTP config missing required fields", source)
        self.assertIn("missing_fields", source)
        self.assertLess(source.index("varify.email.config_missing"), source.index("varify.email.send_start"))
        self.assertNotIn("SMTPPass", source[source.index("varify.email.send_start") :])

    def test_email_sender_consumes_multiline_smtp_replies(self):
        source = read(VARIFY_DIR / "EmailSender.cpp")

        self.assertIn("parse_smtp_status_line", source)
        self.assertIn("IsMultilineReply", source)
        self.assertIn("line.size() > 3 ? line[3]", source)
        self.assertGreaterEqual(source.count("while (more_lines);"), 2)
        self.assertNotRegex(
            source,
            r"return code == expected_code;",
            "SMTP response parsing must consume all lines from multiline replies before sending the next command",
        )

    def test_verify_code_request_does_not_treat_queue_publish_as_email_sent(self):
        source = read(VARIFY_DIR / "VarifyServiceImpl.cpp")
        send_email_body = source.split("bool VarifyServiceImpl::SendEmail", 1)[1].split(
            "grpc::Status VarifyServiceImpl::GetVarifyCode", 1
        )[0]

        self.assertIn("EmailSender::Send", send_email_body)
        self.assertNotIn(
            "PublishVerifyDeliveryTask",
            send_email_body,
            "Publishing to RabbitMQ only queues delivery and must not make /get_varifycode report sent",
        )

    def test_verify_code_request_reports_email_delivery_failure(self):
        source = read(VARIFY_DIR / "VarifyServiceImpl.cpp")
        const_source = read(VARIFY_CONST)
        client_source = read(CLIENT_AUTH_RESPONSES)

        self.assertIn("EmailSendFailed = 1022", const_source)
        self.assertIn("case 1022:", client_source)
        self.assertIn("邮件发送失败", client_source)
        failure_start = source.find("if (!email_ok)")
        self.assertNotEqual(failure_start, -1)
        failure_return = source.find("return grpc::Status::OK;", failure_start)
        self.assertNotEqual(failure_return, -1)
        failure_block = source[failure_start:failure_return]
        self.assertIn("VarifyError::EmailSendFailed", failure_block)
        self.assertNotIn("VarifyError::Success", failure_block)


if __name__ == "__main__":
    unittest.main()
