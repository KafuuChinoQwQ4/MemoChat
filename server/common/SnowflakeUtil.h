#pragma once
#include "snowflake.hpp"
#include <string>

// thread-safe singleton wrapping the sniper00 snowflake template
class SnowflakeUtil {
public:
	static SnowflakeUtil& getInstance() {
		static SnowflakeUtil inst;
		return inst;
	}

	void init(int64_t workerId, int64_t datacenterId) {
		if (initialized_) return;
		sf_.init(workerId, datacenterId);
		initialized_ = true;
	}

	int64_t nextId() { return sf_.nextid(); }

	// formats a Snowflake int64 into a compact string with a prefix letter
	// uses the low 36 bits → fits in a 11-digit number, we take % 1_000_000_000 → 9 digits
	static std::string formatPublicId(int64_t id, char prefix) {
		return std::string(1, prefix) + std::to_string((id & 0x7FFFFFFFFFFF) % 1000000000LL);
	}

	SnowflakeUtil(const SnowflakeUtil&) = delete;
	SnowflakeUtil& operator=(const SnowflakeUtil&) = delete;

private:
	SnowflakeUtil() = default;
	~SnowflakeUtil() = default;

	bool initialized_ = false;
	snowflake<> sf_;
};
