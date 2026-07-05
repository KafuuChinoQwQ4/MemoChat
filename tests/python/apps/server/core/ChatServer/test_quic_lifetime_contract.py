import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
QUIC_SERVER = REPO_ROOT / "apps/server/core/ChatServer/transport/QuicChatServer.cpp"


class QuicLifetimeContractTests(unittest.TestCase):
    def test_msquic_callbacks_use_owned_tokens_instead_of_raw_connection_context(self):
        source = QUIC_SERVER.read_text(encoding="utf-8")

        self.assertIn("struct ConnectionCallbackContext", source)
        self.assertIn("std::shared_ptr<ConnectionContext> connection;", source)
        self.assertIn("struct StreamCallbackContext", source)
        self.assertIn("std::shared_ptr<StreamContext> stream;", source)
        self.assertIn("std::weak_ptr<ConnectionContext> connection;", source)
        self.assertNotIn("ConnectionContext* connection = nullptr;", source)
        self.assertNotIn("delete ctx;", source)

    def test_quic_session_lambdas_do_not_capture_raw_connection_or_owner(self):
        source = QUIC_SERVER.read_text(encoding="utf-8")

        self.assertIn("std::weak_ptr<ConnectionContext> weak_connection = connection_context;", source)
        self.assertIn("[weak_connection](const std::string& payload, short msgid) -> bool", source)
        self.assertIn("[owner_lifetime = connection_context->owner_lifetime](const std::string& session_id)", source)
        self.assertIn("Impl::withOwner(owner_lifetime", source)
        self.assertNotIn("[connection_context](const std::string& payload", source)
        self.assertNotIn("[owner](const std::string& session_id)", source)


if __name__ == "__main__":
    unittest.main()
