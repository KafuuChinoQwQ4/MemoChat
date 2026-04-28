#pragma once
#include <string>
#include "const.h"
#include <iostream>
#include <boost/asio.hpp>
#include <cstdint>
#include <cstring>
#include <memory>

using boost::asio::ip::tcp;
class LogicSystem;

// Fast varint codec for compressing message lengths and IDs.
// Format: 1 byte for values 0-127, 2 bytes for 128-16383, etc.
struct VarintCodec {
	static inline uint32_t encode(uint32_t v, uint8_t* out) {
		uint32_t n = 0;
		while (v >= 0x80) {
			out[n++] = static_cast<uint8_t>(v | 0x80);
			v >>= 7;
		}
		out[n++] = static_cast<uint8_t>(v);
		return n;
	}

	static inline uint32_t decode(const uint8_t*& in, const uint8_t* end) {
		uint32_t v = 0;
		uint32_t shift = 0;
		while (in < end) {
			uint8_t b = *in++;
			v |= (b & 0x7F) << shift;
			if ((b & 0x80) == 0) break;
			shift += 7;
		}
		return v;
	}
};

// Compact frame encoder — prepends a 1-3 byte varint header to payload.
// Frame layout: [varint_payload_len][varint_msg_id][payload...]
// Falls back to fixed 4-byte header when varint savings are minimal.
struct FrameCodec {
	static constexpr uint8_t FLAG_VARINT = 0x01;
	static constexpr uint8_t FLAG_LARGE = 0x02;

	// Returns encoded size (header + payload). header is 1-3 bytes.
	static inline size_t encodedSize(size_t payload_len, size_t msg_id) {
		uint8_t scratch[6];
		uint32_t len_bytes = VarintCodec::encode(static_cast<uint32_t>(payload_len), scratch);
		uint32_t id_bytes = VarintCodec::encode(static_cast<uint32_t>(msg_id), scratch + len_bytes);
		return 1 + len_bytes + id_bytes + payload_len;
	}

	// Encode into existing buffer (assumes caller pre-allocated enough space).
	// Returns total bytes written.
	static inline size_t encodeInPlace(uint8_t* buf, size_t payload_len, uint16_t msg_id) {
		uint8_t* ptr = buf;
		uint8_t len_scratch[6], id_scratch[6];
		uint32_t len_bytes = VarintCodec::encode(static_cast<uint32_t>(payload_len), len_scratch);
		uint32_t id_bytes = VarintCodec::encode(static_cast<uint32_t>(msg_id), id_scratch);
		// Flags byte: bit0=varint, bit1=unused
		uint8_t flags = FLAG_VARINT;
		if (payload_len > 16383 || msg_id > 16383) flags |= FLAG_LARGE;
		*ptr++ = flags;
		::memcpy(ptr, len_scratch, len_bytes); ptr += len_bytes;
		::memcpy(ptr, id_scratch, id_bytes); ptr += id_bytes;
		return static_cast<size_t>(ptr - buf);
	}
};

class MsgNode
{
public:
	MsgNode(short max_len) :_total_len(max_len), _cur_len(0) {
		_data = new char[_total_len + 1]();
		_data[_total_len] = '\0';
	}

	~MsgNode() {
		delete[] _data;
	}

	void Clear() {
		::memset(_data, 0, _total_len);
		_cur_len = 0;
	}

	short _cur_len;
	short _total_len;
	char* _data;
};

class RecvNode :public MsgNode {
	friend class LogicSystem;
public:
	RecvNode(short max_len, short msg_id);
private:
	short _msg_id;
};

class SendNode:public MsgNode {
	friend class LogicSystem;
public:
	SendNode(const char* msg,short max_len, short msg_id);
private:
	short _msg_id;
};

