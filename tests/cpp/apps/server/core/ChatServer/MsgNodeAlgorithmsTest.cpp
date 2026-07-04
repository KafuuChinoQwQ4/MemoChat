#include <gtest/gtest.h>

int MemoChatTestMsgNodeHeaderTotalLength(int id_len, int data_len);
int MemoChatTestMsgNodeSendBufferTotalLength(int payload_len, int header_len);
int MemoChatTestMsgNodeMessageIdOffset();
int MemoChatTestMsgNodeMessageLengthOffset(int id_len);
int MemoChatTestMsgNodePayloadOffset(int id_len, int data_len);

TEST(MsgNodeAlgorithmsTest, ComputesFixedHeaderOffsets)
{
    EXPECT_EQ(MemoChatTestMsgNodeHeaderTotalLength(2, 2), 4);
    EXPECT_EQ(MemoChatTestMsgNodeMessageIdOffset(), 0);
    EXPECT_EQ(MemoChatTestMsgNodeMessageLengthOffset(2), 2);
    EXPECT_EQ(MemoChatTestMsgNodePayloadOffset(2, 2), 4);
}

TEST(MsgNodeAlgorithmsTest, ComputesSendBufferLengths)
{
    EXPECT_EQ(MemoChatTestMsgNodeSendBufferTotalLength(0, 4), 4);
    EXPECT_EQ(MemoChatTestMsgNodeSendBufferTotalLength(12, 4), 16);
    EXPECT_EQ(MemoChatTestMsgNodeSendBufferTotalLength(1024, 4), 1028);
}
