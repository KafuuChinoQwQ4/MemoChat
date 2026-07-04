#include <gtest/gtest.h>

namespace memochat::tests::varify::email_sender
{
int DefaultSmtpPort();
bool ShouldUseImplicitSsl(int port);
bool HasStatusCodePrefix(unsigned long long line_size);
bool IsMultilineReply(unsigned long long line_size, char separator);
bool IsExpectedStatusCode(int actual_code, int expected_code);
} // namespace memochat::tests::varify::email_sender

TEST(EmailSenderAlgorithmsTest, ExposesDefaultSmtpPortAndSslDecision)
{
    EXPECT_EQ(memochat::tests::varify::email_sender::DefaultSmtpPort(), 465);
    EXPECT_TRUE(memochat::tests::varify::email_sender::ShouldUseImplicitSsl(465));
    EXPECT_FALSE(memochat::tests::varify::email_sender::ShouldUseImplicitSsl(25));
    EXPECT_FALSE(memochat::tests::varify::email_sender::ShouldUseImplicitSsl(587));
}

TEST(EmailSenderAlgorithmsTest, PreservesSmtpStatusLineShapeRules)
{
    EXPECT_FALSE(memochat::tests::varify::email_sender::HasStatusCodePrefix(0));
    EXPECT_FALSE(memochat::tests::varify::email_sender::HasStatusCodePrefix(2));
    EXPECT_TRUE(memochat::tests::varify::email_sender::HasStatusCodePrefix(3));

    EXPECT_FALSE(memochat::tests::varify::email_sender::IsMultilineReply(3, '-'));
    EXPECT_TRUE(memochat::tests::varify::email_sender::IsMultilineReply(4, '-'));
    EXPECT_FALSE(memochat::tests::varify::email_sender::IsMultilineReply(4, ' '));
}

TEST(EmailSenderAlgorithmsTest, ComparesExpectedSmtpStatusCode)
{
    EXPECT_TRUE(memochat::tests::varify::email_sender::IsExpectedStatusCode(250, 250));
    EXPECT_FALSE(memochat::tests::varify::email_sender::IsExpectedStatusCode(550, 250));
}
