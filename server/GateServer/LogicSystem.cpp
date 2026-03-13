#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"
#include "MediaStorage.h"
#include "CallService.h"
#include "auth/ChatLoginTicket.h"
#include "cluster/ChatClusterDiscovery.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <atomic>
#include <climits>

namespace {
const char* kMinClientVersion = "2.0.0";
constexpr int kLoginProtocolVersion = 3;

struct GateChatRouteNode {
	std::string name;
	std::string host;
	std::string port;
	int online_count = 0;
	int priority = 0;
};

std::atomic<uint64_t> g_gate_route_rr_counter{0};

bool ParseSemVer(const std::string& ver, int& major, int& minor, int& patch) {
	major = 0;
	minor = 0;
	patch = 0;
	if (ver.empty()) {
		return false;
	}

	std::stringstream ss(ver);
	std::string token;
	std::vector<int> parts;
	while (std::getline(ss, token, '.')) {
		if (token.empty()) {
			return false;
		}
		for (char c : token) {
			if (!std::isdigit(static_cast<unsigned char>(c))) {
				return false;
			}
		}
		parts.push_back(std::stoi(token));
	}
	if (parts.empty() || parts.size() > 3) {
		return false;
	}
	major = parts[0];
	minor = (parts.size() >= 2) ? parts[1] : 0;
	patch = (parts.size() >= 3) ? parts[2] : 0;
	return true;
}

bool IsClientVersionAllowed(const std::string& clientVer, const std::string& minVer) {
	int cMaj = 0, cMin = 0, cPatch = 0;
	int mMaj = 0, mMin = 0, mPatch = 0;
	if (!ParseSemVer(clientVer, cMaj, cMin, cPatch)) {
		return false;
	}
	if (!ParseSemVer(minVer, mMaj, mMin, mPatch)) {
		return true;
	}

	if (cMaj != mMaj) {
		return cMaj > mMaj;
	}
	if (cMin != mMin) {
		return cMin > mMin;
	}
	return cPatch >= mPatch;
}

std::string SanitizeFileName(const std::string& fileName) {
	std::string safe;
	safe.reserve(fileName.size());
	for (char c : fileName) {
		if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.') {
			safe.push_back(c);
		}
		else {
			safe.push_back('_');
		}
	}
	if (safe.empty()) {
		return "file.bin";
	}
	if (safe.size() > 96) {
		safe = safe.substr(safe.size() - 96);
	}
	return safe;
}

bool DecodeBase64(const std::string& input, std::string& output) {
	static const std::string chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	std::vector<int> table(256, -1);
	for (int i = 0; i < static_cast<int>(chars.size()); ++i) {
		table[static_cast<unsigned char>(chars[i])] = i;
	}

	output.clear();
	int val = 0;
	int valb = -8;
	for (unsigned char c : input) {
		if (std::isspace(c)) {
			continue;
		}
		if (c == '=') {
			break;
		}
		if (table[c] == -1) {
			return false;
		}
		val = (val << 6) + table[c];
		valb += 6;
		if (valb >= 0) {
			output.push_back(static_cast<char>((val >> valb) & 0xFF));
			valb -= 8;
		}
	}
	return true;
}

std::string GuessContentType(const std::string& fileName, const std::string& mimeHint) {
	if (!mimeHint.empty()) {
		return mimeHint;
	}
	static const std::unordered_map<std::string, std::string> extMap = {
		{".png", "image/png"},
		{".jpg", "image/jpeg"},
		{".jpeg", "image/jpeg"},
		{".webp", "image/webp"},
		{".bmp", "image/bmp"},
		{".gif", "image/gif"},
		{".txt", "text/plain"},
		{".pdf", "application/pdf"}
	};
	std::string ext;
	const auto dotPos = fileName.find_last_of('.');
	if (dotPos != std::string::npos) {
		ext = fileName.substr(dotPos);
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	}
	const auto it = extMap.find(ext);
	if (it != extMap.end()) {
		return it->second;
	}
	return "application/octet-stream";
}

struct MediaConfig {
	int64_t max_image_bytes = 200LL * 1024 * 1024;
	int64_t max_file_bytes = 20480LL * 1024 * 1024;
	int chunk_size_bytes = 4 * 1024 * 1024;
	int session_expire_sec = 86400;
	std::string storage_provider = "local";
};

int64_t ParseConfigInt64(const std::string& raw, int64_t fallback) {
	if (raw.empty()) {
		return fallback;
	}
	try {
		const int64_t value = std::stoll(raw);
		if (value <= 0) {
			return fallback;
		}
		return value;
	}
	catch (...) {
		return fallback;
	}
}

int ParseConfigInt(const std::string& raw, int fallback) {
	if (raw.empty()) {
		return fallback;
	}
	try {
		const int value = std::stoi(raw);
		if (value <= 0) {
			return fallback;
		}
		return value;
	}
	catch (...) {
		return fallback;
	}
}

MediaConfig LoadMediaConfig() {
	MediaConfig cfg;
	auto media = ConfigMgr::Inst()["Media"];
	cfg.max_image_bytes = ParseConfigInt64(media["MaxImageMB"], 200) * 1024 * 1024;
	cfg.max_file_bytes = ParseConfigInt64(media["MaxFileMB"], 20480) * 1024 * 1024;
	cfg.chunk_size_bytes = ParseConfigInt(media["ChunkSizeKB"], 4096) * 1024;
	cfg.session_expire_sec = ParseConfigInt(media["SessionExpireSec"], 86400);
	const std::string provider = media["StorageProvider"];
	if (!provider.empty()) {
		cfg.storage_provider = provider;
	}
	if (cfg.chunk_size_bytes < 256 * 1024) {
		cfg.chunk_size_bytes = 256 * 1024;
	}
	if (cfg.chunk_size_bytes > 4 * 1024 * 1024) {
		cfg.chunk_size_bytes = 4 * 1024 * 1024;
	}
	return cfg;
}

int64_t NowMs() {
	return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());
}

std::filesystem::path UploadRoot() {
	return std::filesystem::current_path() / "uploads";
}

std::filesystem::path SessionRoot() {
	return UploadRoot() / "sessions";
}

std::filesystem::path ChunkRoot() {
	return UploadRoot() / "chunks";
}

std::filesystem::path SessionPathFor(const std::string& upload_id) {
	return SessionRoot() / (upload_id + ".json");
}

std::filesystem::path ChunkDirFor(const std::string& upload_id) {
	return ChunkRoot() / upload_id;
}

bool ResolveLegacyMediaPath(const std::string& legacy_file, std::filesystem::path& full_path) {
	if (legacy_file.empty()) {
		return false;
	}

	const std::filesystem::path rel = std::filesystem::path(legacy_file).lexically_normal();
	if (rel.is_absolute()) {
		return false;
	}
	const std::string rel_str = rel.generic_string();
	if (rel_str.find("..") != std::string::npos) {
		return false;
	}

	std::error_code ec;
	const std::filesystem::path upload_root = UploadRoot();
	const std::filesystem::path by_relative = upload_root / rel;
	if (std::filesystem::exists(by_relative, ec) && !ec && std::filesystem::is_regular_file(by_relative, ec)) {
		full_path = by_relative;
		return true;
	}
	ec.clear();

	const std::filesystem::path by_filename = upload_root / rel.filename();
	if (std::filesystem::exists(by_filename, ec) && !ec && std::filesystem::is_regular_file(by_filename, ec)) {
		full_path = by_filename;
		return true;
	}
	ec.clear();

	const std::filesystem::path asset_root = upload_root / "assets";
	if (!std::filesystem::exists(asset_root, ec) || ec) {
		return false;
	}
	for (const auto& entry : std::filesystem::recursive_directory_iterator(asset_root, ec)) {
		if (ec) {
			return false;
		}
		if (!entry.is_regular_file()) {
			continue;
		}
		if (entry.path().filename() == rel.filename()) {
			full_path = entry.path();
			return true;
		}
	}
	return false;
}

bool SaveJsonFile(const std::filesystem::path& path, const Json::Value& root) {
	std::error_code ec;
	std::filesystem::create_directories(path.parent_path(), ec);
	if (ec) {
		return false;
	}
	std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
	if (!ofs.is_open()) {
		return false;
	}
	ofs << root.toStyledString();
	return ofs.good();
}

bool LoadJsonFile(const std::filesystem::path& path, Json::Value& root) {
	if (!std::filesystem::exists(path)) {
		return false;
	}
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs.is_open()) {
		return false;
	}
	std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	Json::Reader reader;
	return reader.parse(json, root);
}

bool ValidateUserToken(int uid, const std::string& token) {
	if (uid <= 0 || token.empty()) {
		return false;
	}
	const std::string token_key = USERTOKENPREFIX + std::to_string(uid);
	std::string token_value;
	if (!RedisMgr::GetInstance()->Get(token_key, token_value)) {
		return false;
	}
	return !token_value.empty() && token_value == token;
}

std::set<int> ListUploadedChunkIndexes(const std::filesystem::path& chunk_dir) {
	std::set<int> indexes;
	if (!std::filesystem::exists(chunk_dir)) {
		return indexes;
	}
	for (const auto& entry : std::filesystem::directory_iterator(chunk_dir)) {
		if (!entry.is_regular_file()) {
			continue;
		}
		const std::string stem = entry.path().stem().string();
		if (stem.empty()) {
			continue;
		}
		bool digits = true;
		for (char c : stem) {
			if (!std::isdigit(static_cast<unsigned char>(c))) {
				digits = false;
				break;
			}
		}
		if (!digits) {
			continue;
		}
		try {
			indexes.insert(std::stoi(stem));
		}
		catch (...) {
		}
	}
	return indexes;
}

IMediaStorage& MediaStorageFor(const std::string& provider) {
	static LocalMediaStorage local_storage;
	(void)provider;
	return local_storage;
}

bool IsMediaTypeImage(const std::string& media_type) {
	return media_type == "image";
}

std::string NewIdString() {
	return boost::uuids::to_string(boost::uuids::random_generator()());
}

std::string GetChatAuthSecret() {
	auto& cfg = ConfigMgr::Inst();
	auto secret = cfg.GetValue("ChatAuth", "HmacSecret");
	if (secret.empty()) {
		secret = "memochat-dev-chat-secret";
	}
	return secret;
}

int GetChatTicketTtlSec() {
	auto& cfg = ConfigMgr::Inst();
	const auto ttl = cfg.GetValue("ChatAuth", "TicketTtlSec");
	if (ttl.empty()) {
		return 20;
	}
	return std::max(5, std::atoi(ttl.c_str()));
}

int GetLoginCacheTtlSec() {
	auto& cfg = ConfigMgr::Inst();
	const auto ttl = cfg.GetValue("LoginCache", "TtlSec");
	if (ttl.empty()) {
		return 3600;
	}
	return std::max(60, std::atoi(ttl.c_str()));
}

std::string BuildLoginCacheKey(const std::string& email) {
	return "ulogin_profile_" + email;
}

std::string BuildLoginCacheUidKey(int uid) {
	return "ulogin_profile_uid_" + std::to_string(uid);
}

std::string DecodeLegacyXorPwd(const std::string& input) {
	unsigned int xor_code = static_cast<unsigned int>(input.size() % 255);
	std::string decoded = input;
	for (size_t i = 0; i < decoded.size(); ++i) {
		decoded[i] = static_cast<char>(static_cast<unsigned char>(decoded[i]) ^ xor_code);
	}
	return decoded;
}

bool TryLoadCachedLoginProfile(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	std::string cached_json;
	if (!RedisMgr::GetInstance()->Get(BuildLoginCacheKey(email), cached_json) || cached_json.empty()) {
		return false;
	}

	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(cached_json, root) || !root.isObject()) {
		return false;
	}

	const auto cached_pwd = root.get("pwd", "").asString();
	if (cached_pwd.empty()) {
		return false;
	}
	if (pwd != cached_pwd && DecodeLegacyXorPwd(pwd) != cached_pwd) {
		return false;
	}

	userInfo.pwd = cached_pwd;
	userInfo.name = root.get("name", "").asString();
	userInfo.email = root.get("email", "").asString();
	userInfo.uid = root.get("uid", 0).asInt();
	userInfo.user_id = root.get("user_id", "").asString();
	userInfo.nick = root.get("nick", "").asString();
	userInfo.icon = root.get("icon", "").asString();
	userInfo.desc = root.get("desc", "").asString();
	userInfo.sex = root.get("sex", 0).asInt();
	return userInfo.uid > 0;
}

void CacheLoginProfile(const std::string& email, const UserInfo& userInfo) {
	Json::Value root(Json::objectValue);
	root["uid"] = userInfo.uid;
	root["pwd"] = userInfo.pwd;
	root["name"] = userInfo.name;
	root["email"] = userInfo.email;
	root["user_id"] = userInfo.user_id;
	root["nick"] = userInfo.nick;
	root["icon"] = userInfo.icon;
	root["desc"] = userInfo.desc;
	root["sex"] = userInfo.sex;
	const auto ttl = GetLoginCacheTtlSec();
	RedisMgr::GetInstance()->SetEx(BuildLoginCacheKey(email), root.toStyledString(), ttl);
	if (userInfo.uid > 0) {
		RedisMgr::GetInstance()->SetEx(BuildLoginCacheUidKey(userInfo.uid), email, ttl);
	}
}

void InvalidateLoginCacheByEmail(const std::string& email) {
	if (email.empty()) {
		return;
	}
	RedisMgr::GetInstance()->Del(BuildLoginCacheKey(email));
}

void InvalidateLoginCacheByUid(int uid) {
	if (uid <= 0) {
		return;
	}
	std::string email;
	if (RedisMgr::GetInstance()->Get(BuildLoginCacheUidKey(uid), email) && !email.empty()) {
		RedisMgr::GetInstance()->Del(BuildLoginCacheKey(email));
	}
	RedisMgr::GetInstance()->Del(BuildLoginCacheUidKey(uid));
}

std::vector<GateChatRouteNode> LoadGateChatRouteNodes(std::vector<std::string>* load_snapshot = nullptr,
                                                      std::vector<std::string>* least_loaded_snapshot = nullptr) {
	if (load_snapshot) {
		load_snapshot->clear();
	}
	if (least_loaded_snapshot) {
		least_loaded_snapshot->clear();
	}
	try {
		auto& cfg = ConfigMgr::Inst();
		const auto cluster = memochat::cluster::LoadStaticChatClusterConfig(
			[&cfg](const std::string& section, const std::string& key) {
				return cfg.GetValue(section, key);
			});
		std::vector<GateChatRouteNode> nodes;
		nodes.reserve(cluster.enabledNodes().size());
		int min_online = INT_MAX;
		for (const auto& node : cluster.enabledNodes()) {
			GateChatRouteNode route_node;
			route_node.name = node.name;
			route_node.host = node.tcp_host;
			route_node.port = node.tcp_port;
			const int online_count = 0;
			route_node.online_count = online_count;
			nodes.push_back(route_node);
			min_online = std::min(min_online, online_count);
			if (load_snapshot) {
				std::ostringstream one;
				one << node.name << "=" << online_count;
				load_snapshot->push_back(one.str());
			}
		}
		std::sort(nodes.begin(), nodes.end(), [](const GateChatRouteNode& lhs, const GateChatRouteNode& rhs) {
			if (lhs.online_count != rhs.online_count) {
				return lhs.online_count < rhs.online_count;
			}
			return lhs.name < rhs.name;
		});
		int priority = 0;
		for (auto& node : nodes) {
			node.priority = priority++;
			if (node.online_count == min_online && least_loaded_snapshot) {
				least_loaded_snapshot->push_back(node.name);
			}
		}
		if (!nodes.empty()) {
			const auto next_index = static_cast<size_t>(g_gate_route_rr_counter.fetch_add(1, std::memory_order_relaxed));
			std::stable_sort(nodes.begin(), nodes.end(), [next_index, min_online](const GateChatRouteNode& lhs, const GateChatRouteNode& rhs) {
				const bool lhs_least = lhs.online_count == min_online;
				const bool rhs_least = rhs.online_count == min_online;
				if (lhs_least != rhs_least) {
					return lhs_least > rhs_least;
				}
				if (lhs_least && rhs_least) {
					return ((lhs.priority + static_cast<int>(next_index)) % 1024) < ((rhs.priority + static_cast<int>(next_index)) % 1024);
				}
				return lhs.priority < rhs.priority;
			});
		}
		return nodes;
	}
	catch (const std::exception& ex) {
		memolog::LogError("gate.route_select.config_error", "failed to load local chat route config",
			{
				{"error_type", "cluster_config"},
				{"error", ex.what()}
			});
		if (load_snapshot) {
			load_snapshot->push_back(std::string("config_error:") + ex.what());
		}
		return {};
	}
}
}

LogicSystem::LogicSystem() {
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req " << std::endl;
		int i = 0;
		for (auto& elem : connection->_get_params) {
			i++;
			beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
			beast::ostream(connection->_response.body()) << ", " <<  " value is " << elem.second << std::endl;
		}

		connection->_response.set(http::field::content_type, "text/plain");
	});

	RegPost("/test_procedure", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!src_root.isMember("email")) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		int uid = 0;
		std::string name = "";
		MysqlMgr::GetInstance()->TestProcedure(email, uid, name);
		cout << "email is " << email << endl;
		root["error"] = ErrorCodes::Success;
		root["email"] = src_root["email"];
		root["name"] = name;
		root["uid"] = uid;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		
	});

	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		memolog::TraceContext::SetTraceId(connection->_trace_id);
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			memolog::LogWarn("gate.get_varifycode.invalid_json", "request json parse failed");
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!src_root.isMember("email")) {
			memolog::LogWarn("gate.get_varifycode.invalid_body", "email is missing");
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
		root["error"] = rsp.error();
		root["email"] = src_root["email"];
		memolog::LogInfo("gate.get_varifycode", "verify code requested",
			{
				{"route", "/get_varifycode"},
				{"email", email},
				{"error_code", std::to_string(rsp.error())}
			});
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		memolog::TraceContext::SetTraceId(connection->_trace_id);
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			memolog::LogWarn("gate.user_register.invalid_json", "request json parse failed");
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();
		auto icon = src_root["icon"].asString();

		if (pwd != confirm) {
			memolog::LogWarn("gate.user_register.failed", "password mismatch", { {"email", email} });
			root["error"] = ErrorCodes::PasswdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}


		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX+src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			memolog::LogWarn("gate.user_register.failed", "verify code expired", { {"email", email} });
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			memolog::LogWarn("gate.user_register.failed", "verify code mismatch", { {"email", email} });
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}


		int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd, icon);
		if (uid == 0 || uid == -1) {
			memolog::LogWarn("gate.user_register.failed", "user or email exists", { {"email", email}, {"name", name} });
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		root["error"] = 0;
		root["uid"] = uid;
		root["user_id"] = MysqlMgr::GetInstance()->GetUserPublicId(uid);
		root["email"] = email;
		root ["user"]= name;
		root["passwd"] = pwd;
		root["confirm"] = confirm;
		root["icon"] = icon;
		root["varifycode"] = src_root["varifycode"].asString();
		UserInfo cached_user;
		cached_user.uid = uid;
		cached_user.user_id = root["user_id"].asString();
		cached_user.name = name;
		cached_user.email = email;
		cached_user.pwd = pwd;
		cached_user.nick = name;
		cached_user.icon = icon;
		cached_user.desc = "";
		cached_user.sex = src_root.get("sex", 0).asInt();
		CacheLoginProfile(email, cached_user);
		memolog::LogInfo("gate.user_register", "user registered",
			{ {"email", email}, {"uid", std::to_string(uid)} });
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});


	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		memolog::TraceContext::SetTraceId(connection->_trace_id);
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			memolog::LogWarn("gate.reset_pwd.invalid_json", "request json parse failed");
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();


		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			memolog::LogWarn("gate.reset_pwd.failed", "verify code expired", { {"email", email} });
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			memolog::LogWarn("gate.reset_pwd.failed", "verify code mismatch", { {"email", email} });
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
		if (!email_valid) {
			memolog::LogWarn("gate.reset_pwd.failed", "user email mismatch", { {"email", email}, {"name", name} });
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}


		bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
		if (!b_up) {
			memolog::LogWarn("gate.reset_pwd.failed", "password update failed", { {"email", email} });
			root["error"] = ErrorCodes::PasswdUpFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		memolog::LogInfo("gate.reset_pwd", "password updated", { {"email", email} });
		InvalidateLoginCacheByEmail(email);
		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});


	RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		memolog::TraceContext::SetTraceId(connection->_trace_id);
		connection->_response.set(http::field::content_type, "text/json");
		const auto login_start_ms = NowMs();
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		root["trace_id"] = connection->_trace_id;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			memolog::LogWarn("gate.user_login.invalid_json", "request json parse failed");
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
		auto client_ver = src_root.get("client_ver", "").asString();
		root["min_version"] = kMinClientVersion;
		root["feature_group_chat"] = true;
		if (!IsClientVersionAllowed(client_ver, kMinClientVersion)) {
			root["error"] = ErrorCodes::ClientVersionTooLow;
			memolog::LogWarn("gate.user_login.failed", "client version too low",
				{ {"email", email}, {"error_code", std::to_string(ErrorCodes::ClientVersionTooLow)} });
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		UserInfo userInfo;
		const auto mysql_start_ms = NowMs();
		bool login_cache_hit = TryLoadCachedLoginProfile(email, pwd, userInfo);
		bool pwd_valid = login_cache_hit;
		int64_t mysql_check_pwd_ms = 0;
		if (!pwd_valid) {
			pwd_valid = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
			mysql_check_pwd_ms = NowMs() - mysql_start_ms;
			if (pwd_valid) {
				CacheLoginProfile(email, userInfo);
			}
		}
		if (!pwd_valid) {
			memolog::LogWarn("gate.user_login.failed", "password invalid",
				{ {"email", email}, {"error_code", std::to_string(ErrorCodes::PasswdInvalid)} });
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		std::vector<std::string> server_load_snapshot;
		std::vector<std::string> least_loaded_servers;
		const auto route_start_ms = NowMs();
		const auto route_nodes = LoadGateChatRouteNodes(&server_load_snapshot, &least_loaded_servers);
		const auto route_select_ms = NowMs() - route_start_ms;
		if (route_nodes.empty()) {
			memolog::LogWarn("gate.user_login.failed", "no chat server available",
				{ {"uid", std::to_string(userInfo.uid)}, {"error_code", std::to_string(ErrorCodes::RPCFailed)} });
			root["error"] = ErrorCodes::RPCFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		const auto ticket_start_ms = NowMs();
		std::string http_token;
		const std::string token_key = USERTOKENPREFIX + std::to_string(userInfo.uid);
		if (!RedisMgr::GetInstance()->Get(token_key, http_token) || http_token.empty()) {
			http_token = NewIdString();
			RedisMgr::GetInstance()->Set(token_key, http_token);
		}
		memochat::auth::ChatLoginTicketClaims claims;
		claims.uid = userInfo.uid;
		claims.user_id = userInfo.user_id;
		claims.name = userInfo.name;
		claims.nick = userInfo.nick;
		claims.icon = userInfo.icon;
		claims.desc = userInfo.desc;
		claims.email = userInfo.email;
		claims.sex = userInfo.sex;
		claims.target_server = route_nodes.front().name;
		claims.protocol_version = kLoginProtocolVersion;
		claims.issued_at_ms = NowMs();
		claims.expire_at_ms = claims.issued_at_ms + static_cast<int64_t>(GetChatTicketTtlSec()) * 1000;
		const std::string login_ticket = memochat::auth::EncodeTicket(claims, GetChatAuthSecret());
		const auto ticket_issue_ms = NowMs() - ticket_start_ms;
		if (login_ticket.empty()) {
			root["error"] = ErrorCodes::RPCFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		memolog::TraceContext::SetUid(std::to_string(userInfo.uid));
		root["error"] = 0;
		root["protocol_version"] = kLoginProtocolVersion;
		root["email"] = email;
		root["uid"] = userInfo.uid;
		root["user_id"] = userInfo.user_id;
		root["token"] = http_token;
		root["host"] = route_nodes.front().host;
		root["port"] = route_nodes.front().port;
		root["login_ticket"] = login_ticket;
		root["ticket_expire_ms"] = static_cast<Json::Int64>(claims.expire_at_ms);
		root["user_profile"]["uid"] = userInfo.uid;
		root["user_profile"]["user_id"] = userInfo.user_id;
		root["user_profile"]["name"] = userInfo.name;
		root["user_profile"]["nick"] = userInfo.nick;
		root["user_profile"]["icon"] = userInfo.icon;
		root["user_profile"]["desc"] = userInfo.desc;
		root["user_profile"]["email"] = userInfo.email;
		root["user_profile"]["sex"] = userInfo.sex;
		for (const auto& route_node : route_nodes) {
			Json::Value endpoint;
			endpoint["host"] = route_node.host;
			endpoint["port"] = route_node.port;
			endpoint["server_name"] = route_node.name;
			endpoint["priority"] = route_node.priority;
			root["chat_endpoints"].append(endpoint);
		}
		root["stage_metrics"]["mysql_check_pwd_ms"] = static_cast<Json::Int64>(mysql_check_pwd_ms);
		root["stage_metrics"]["route_select_ms"] = static_cast<Json::Int64>(route_select_ms);
		root["stage_metrics"]["ticket_issue_ms"] = static_cast<Json::Int64>(ticket_issue_ms);
		root["stage_metrics"]["user_login_total_ms"] = static_cast<Json::Int64>(NowMs() - login_start_ms);
		memolog::LogInfo("gate.user_login", "user login succeeded",
			{
				{"uid", std::to_string(userInfo.uid)},
				{"route", "/user_login"},
				{"chat_host", route_nodes.front().host},
				{"chat_port", route_nodes.front().port},
				{"chat_server", route_nodes.front().name},
				{"login_cache_hit", login_cache_hit ? "true" : "false"},
				{"mysql_check_pwd_ms", std::to_string(mysql_check_pwd_ms)},
				{"route_select_ms", std::to_string(route_select_ms)},
				{"ticket_issue_ms", std::to_string(ticket_issue_ms)},
				{"user_login_total_ms", std::to_string(NowMs() - login_start_ms)},
				{"server_loads", [&server_load_snapshot]() {
					std::ostringstream oss;
					for (size_t i = 0; i < server_load_snapshot.size(); ++i) {
						if (i > 0) {
							oss << ",";
						}
						oss << server_load_snapshot[i];
					}
					return oss.str();
				}()}
			});
		memolog::LogInfo("login.stage.summary", "gate login stage summary",
			{
				{"uid", std::to_string(userInfo.uid)},
				{"login_cache_hit", login_cache_hit ? "true" : "false"},
				{"mysql_check_pwd_ms", std::to_string(mysql_check_pwd_ms)},
				{"route_select_ms", std::to_string(route_select_ms)},
				{"ticket_issue_ms", std::to_string(ticket_issue_ms)},
				{"user_login_total_ms", std::to_string(NowMs() - login_start_ms)}
			});
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	RegPost("/upload_media_init", [](std::shared_ptr<HttpConnection> connection) {
		const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Value src_root;
		Json::Reader reader;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			root["message"] = "invalid json";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const int uid = src_root.get("uid", 0).asInt();
		const std::string token = src_root.get("token", "").asString();
		std::string media_type = src_root.get("media_type", "file").asString();
		if (media_type.empty()) {
			media_type = "file";
		}
		const std::string file_name = SanitizeFileName(src_root.get("file_name", "").asString());
		std::string mime = src_root.get("mime", "").asString();
		const int64_t file_size = src_root.get("file_size", 0).asInt64();
		if (uid <= 0 || file_name.empty() || file_size <= 0 || !ValidateUserToken(uid, token)) {
			root["error"] = ErrorCodes::TokenInvalid;
			root["message"] = "token invalid or params invalid";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const MediaConfig media_cfg = LoadMediaConfig();
		const int64_t limit = IsMediaTypeImage(media_type) ? media_cfg.max_image_bytes : media_cfg.max_file_bytes;
		if (file_size > limit) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "file too large";
			root["limit_bytes"] = static_cast<Json::Int64>(limit);
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		if (mime.empty()) {
			mime = GuessContentType(file_name, "");
		}
		const std::string upload_id = NewIdString();
		const int chunk_size = media_cfg.chunk_size_bytes;
		const int total_chunks = static_cast<int>((file_size + chunk_size - 1) / chunk_size);
		std::error_code ec;
		std::filesystem::create_directories(ChunkDirFor(upload_id), ec);
		if (ec) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "create chunk dir failed";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		Json::Value session;
		session["uid"] = uid;
		session["upload_id"] = upload_id;
		session["media_type"] = media_type;
		session["file_name"] = file_name;
		session["mime"] = mime;
		session["file_size"] = static_cast<Json::Int64>(file_size);
		session["chunk_size"] = chunk_size;
		session["total_chunks"] = total_chunks;
		session["created_at"] = static_cast<Json::Int64>(NowMs());
		session["expires_at"] = static_cast<Json::Int64>(NowMs() + static_cast<int64_t>(media_cfg.session_expire_sec) * 1000);
		session["storage_provider"] = media_cfg.storage_provider;
		if (!SaveJsonFile(SessionPathFor(upload_id), session)) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "create upload session failed";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		root["error"] = ErrorCodes::Success;
		root["upload_id"] = upload_id;
		root["chunk_size"] = chunk_size;
		root["total_chunks"] = total_chunks;
		root["uploaded_chunks"] = Json::arrayValue;
		beast::ostream(connection->_response.body()) << root.toStyledString();
		return true;
	});

	RegPost("/upload_media_chunk", [](std::shared_ptr<HttpConnection> connection) {
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		int uid = 0;
		int index = -1;
		std::string token;
		std::string upload_id;
		std::string binary;

		std::string content_type;
		const auto ct_it = connection->_request.find(http::field::content_type);
		if (ct_it != connection->_request.end()) {
			content_type = std::string(ct_it->value().data(), ct_it->value().size());
			std::transform(content_type.begin(), content_type.end(), content_type.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
				});
		}

		if (content_type.find("application/json") != std::string::npos) {
			const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
			Json::Value src_root;
			Json::Reader reader;
			if (!reader.parse(body_str, src_root)) {
				root["error"] = ErrorCodes::Error_Json;
				root["message"] = "invalid json";
				beast::ostream(connection->_response.body()) << root.toStyledString();
				return true;
			}

			uid = src_root.get("uid", 0).asInt();
			token = src_root.get("token", "").asString();
			upload_id = src_root.get("upload_id", "").asString();
			index = src_root.get("index", -1).asInt();
			const std::string encoded = src_root.get("data_base64", "").asString();
			if (encoded.empty() || !DecodeBase64(encoded, binary)) {
				root["error"] = ErrorCodes::Error_Json;
				root["message"] = "base64 decode failed";
				beast::ostream(connection->_response.body()) << root.toStyledString();
				return true;
			}
		}
		else {
			const auto read_header = [&connection](const char* key) -> std::string {
				const auto it = connection->_request.find(key);
				if (it == connection->_request.end()) {
					return "";
				}
				return std::string(it->value().data(), it->value().size());
				};
			uid = std::atoi(read_header("X-Uid").c_str());
			token = read_header("X-Token");
			upload_id = read_header("X-Upload-Id");
			index = std::atoi(read_header("X-Chunk-Index").c_str());
			binary = boost::beast::buffers_to_string(connection->_request.body().data());
		}

		if (uid <= 0 || upload_id.empty() || index < 0 || binary.empty() || !ValidateUserToken(uid, token)) {
			root["error"] = ErrorCodes::TokenInvalid;
			root["message"] = "token invalid or params invalid";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		Json::Value session;
		if (!LoadJsonFile(SessionPathFor(upload_id), session)) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "upload session not found";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}
		if (session.get("uid", 0).asInt() != uid) {
			root["error"] = ErrorCodes::TokenInvalid;
			root["message"] = "session uid mismatch";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}
		const int64_t expires_at = session.get("expires_at", 0).asInt64();
		if (expires_at > 0 && NowMs() > expires_at) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "upload session expired";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const int total_chunks = session.get("total_chunks", 0).asInt();
		const int chunk_size = session.get("chunk_size", 0).asInt();
		if (index >= total_chunks || total_chunks <= 0 || chunk_size <= 0) {
			root["error"] = ErrorCodes::Error_Json;
			root["message"] = "invalid chunk index";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		if (binary.empty() || static_cast<int>(binary.size()) > chunk_size) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "invalid chunk size";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const std::filesystem::path chunk_dir = ChunkDirFor(upload_id);
		std::error_code ec;
		std::filesystem::create_directories(chunk_dir, ec);
		if (ec) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "create chunk dir failed";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const std::filesystem::path chunk_path = chunk_dir / (std::to_string(index) + ".part");
		std::ofstream ofs(chunk_path, std::ios::binary | std::ios::trunc);
		if (!ofs.is_open()) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "write chunk failed";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}
		ofs.write(binary.data(), static_cast<std::streamsize>(binary.size()));
		ofs.close();

		root["error"] = ErrorCodes::Success;
		root["upload_id"] = upload_id;
		root["index"] = index;
		root["size"] = static_cast<Json::Int64>(binary.size());
		beast::ostream(connection->_response.body()) << root.toStyledString();
		return true;
	});

	RegGet("/upload_media_status", [](std::shared_ptr<HttpConnection> connection) {
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		const int uid = std::atoi(connection->_get_params["uid"].c_str());
		const std::string token = connection->_get_params["token"];
		const std::string upload_id = connection->_get_params["upload_id"];
		if (uid <= 0 || token.empty() || upload_id.empty() || !ValidateUserToken(uid, token)) {
			root["error"] = ErrorCodes::TokenInvalid;
			root["message"] = "token invalid or params invalid";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		Json::Value session;
		if (!LoadJsonFile(SessionPathFor(upload_id), session)) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "upload session not found";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}
		if (session.get("uid", 0).asInt() != uid) {
			root["error"] = ErrorCodes::TokenInvalid;
			root["message"] = "session uid mismatch";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		root["error"] = ErrorCodes::Success;
		root["upload_id"] = upload_id;
		root["total_chunks"] = session.get("total_chunks", 0).asInt();
		root["chunk_size"] = session.get("chunk_size", 0).asInt();
		root["uploaded_chunks"] = Json::arrayValue;
		for (int idx : ListUploadedChunkIndexes(ChunkDirFor(upload_id))) {
			root["uploaded_chunks"].append(idx);
		}
		beast::ostream(connection->_response.body()) << root.toStyledString();
		return true;
	});

	RegPost("/upload_media_complete", [](std::shared_ptr<HttpConnection> connection) {
		const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Value src_root;
		Json::Reader reader;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			root["message"] = "invalid json";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const int uid = src_root.get("uid", 0).asInt();
		const std::string token = src_root.get("token", "").asString();
		const std::string upload_id = src_root.get("upload_id", "").asString();
		if (uid <= 0 || token.empty() || upload_id.empty() || !ValidateUserToken(uid, token)) {
			root["error"] = ErrorCodes::TokenInvalid;
			root["message"] = "token invalid or params invalid";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		Json::Value session;
		if (!LoadJsonFile(SessionPathFor(upload_id), session)) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "upload session not found";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}
		if (session.get("uid", 0).asInt() != uid) {
			root["error"] = ErrorCodes::TokenInvalid;
			root["message"] = "session uid mismatch";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const int total_chunks = session.get("total_chunks", 0).asInt();
		const int chunk_size = session.get("chunk_size", 0).asInt();
		const int64_t file_size = session.get("file_size", 0).asInt64();
		const std::string file_name = session.get("file_name", "").asString();
		const std::string media_type = session.get("media_type", "file").asString();
		const std::string mime = session.get("mime", "").asString();
		const std::string storage_provider = session.get("storage_provider", "local").asString();
		if (total_chunks <= 0 || chunk_size <= 0 || file_size <= 0 || file_name.empty()) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "invalid upload session";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const std::filesystem::path chunk_dir = ChunkDirFor(upload_id);
		const std::set<int> uploaded = ListUploadedChunkIndexes(chunk_dir);
		for (int i = 0; i < total_chunks; ++i) {
			if (uploaded.find(i) == uploaded.end()) {
				root["error"] = ErrorCodes::MediaUploadFailed;
				root["message"] = "chunks not complete";
				root["missing_index"] = i;
				beast::ostream(connection->_response.body()) << root.toStyledString();
				return true;
			}
		}

		const std::filesystem::path merged_path = chunk_dir / "merged.tmp";
		{
			std::ofstream merged(merged_path, std::ios::binary | std::ios::trunc);
			if (!merged.is_open()) {
				root["error"] = ErrorCodes::MediaUploadFailed;
				root["message"] = "create merged file failed";
				beast::ostream(connection->_response.body()) << root.toStyledString();
				return true;
			}
			for (int i = 0; i < total_chunks; ++i) {
				const std::filesystem::path one_path = chunk_dir / (std::to_string(i) + ".part");
				std::ifstream one( one_path, std::ios::binary);
				if (!one.is_open()) {
					root["error"] = ErrorCodes::MediaUploadFailed;
					root["message"] = "open chunk failed";
					root["chunk_index"] = i;
					beast::ostream(connection->_response.body()) << root.toStyledString();
					return true;
				}
				merged << one.rdbuf();
			}
			merged.flush();
			if (!merged.good()) {
				root["error"] = ErrorCodes::MediaUploadFailed;
				root["message"] = "merge chunks failed";
				beast::ostream(connection->_response.body()) << root.toStyledString();
				return true;
			}
		}

		std::error_code ec;
		const int64_t merged_size = static_cast<int64_t>(std::filesystem::file_size(merged_path, ec));
		if (ec || merged_size != file_size) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "merged file size mismatch";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const std::string media_key = NewIdString();
		std::string storage_path;
		std::string storage_error;
		IMediaStorage& storage = MediaStorageFor(storage_provider);
		if (!storage.StoreMergedFile(media_key, file_name, merged_path, storage_path, storage_error)) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = storage_error.empty() ? "persist media failed" : storage_error;
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		MediaAssetInfo asset;
		asset.media_key = media_key;
		asset.owner_uid = uid;
		asset.media_type = media_type;
		asset.origin_file_name = file_name;
		asset.mime = mime;
		asset.size_bytes = merged_size;
		asset.storage_provider = storage_provider;
		asset.storage_path = storage_path;
		asset.created_at_ms = NowMs();
		asset.deleted_at_ms = 0;
		asset.status = 1;
		if (!MysqlMgr::GetInstance()->InsertMediaAsset(asset)) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "save media metadata failed";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		std::filesystem::remove(SessionPathFor(upload_id), ec);
		std::filesystem::remove_all(chunk_dir, ec);

		root["error"] = ErrorCodes::Success;
		root["media_key"] = media_key;
		root["media_type"] = media_type;
		root["file_name"] = file_name;
		root["mime"] = mime;
		root["size"] = static_cast<Json::Int64>(merged_size);
		root["url"] = std::string("/media/download?asset=") + media_key;
		beast::ostream(connection->_response.body()) << root.toStyledString();
		return true;
	});

	RegPost("/upload_media", [](std::shared_ptr<HttpConnection> connection) {
		const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Value src_root;
		Json::Reader reader;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			root["message"] = "invalid json";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const int uid = src_root.get("uid", 0).asInt();
		const std::string token = src_root.get("token", "").asString();
		std::string media_type = src_root.get("media_type", "file").asString();
		if (media_type.empty()) {
			media_type = "file";
		}
		const std::string file_name = SanitizeFileName(src_root.get("file_name", "").asString());
		std::string mime = src_root.get("mime", "").asString();
		const std::string encoded = src_root.get("data_base64", "").asString();
		if (uid <= 0 || file_name.empty() || encoded.empty() || !ValidateUserToken(uid, token)) {
			root["error"] = ErrorCodes::TokenInvalid;
			root["message"] = "token invalid or params invalid";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		std::string binary;
		if (!DecodeBase64(encoded, binary)) {
			root["error"] = ErrorCodes::Error_Json;
			root["message"] = "base64 decode failed";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}
		if (binary.empty()) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "file empty";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const MediaConfig media_cfg = LoadMediaConfig();
		const int64_t limit = IsMediaTypeImage(media_type) ? media_cfg.max_image_bytes : media_cfg.max_file_bytes;
		if (static_cast<int64_t>(binary.size()) > limit) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "file too large";
			root["limit_bytes"] = static_cast<Json::Int64>(limit);
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}
		if (mime.empty()) {
			mime = GuessContentType(file_name, "");
		}

		const std::string media_key = NewIdString();
		const std::filesystem::path temp_dir = ChunkRoot() / ("legacy_" + media_key);
		std::error_code ec;
		std::filesystem::create_directories(temp_dir, ec);
		if (ec) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "create temp dir failed";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}
		const std::filesystem::path temp_file = temp_dir / "merged.tmp";
		{
			std::ofstream ofs(temp_file, std::ios::binary | std::ios::trunc);
			if (!ofs.is_open()) {
				root["error"] = ErrorCodes::MediaUploadFailed;
				root["message"] = "open temp file failed";
				beast::ostream(connection->_response.body()) << root.toStyledString();
				return true;
			}
			ofs.write(binary.data(), static_cast<std::streamsize>(binary.size()));
		}

		std::string storage_path;
		std::string storage_error;
		IMediaStorage& storage = MediaStorageFor(media_cfg.storage_provider);
		if (!storage.StoreMergedFile(media_key, file_name, temp_file, storage_path, storage_error)) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = storage_error.empty() ? "persist media failed" : storage_error;
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		MediaAssetInfo asset;
		asset.media_key = media_key;
		asset.owner_uid = uid;
		asset.media_type = media_type;
		asset.origin_file_name = file_name;
		asset.mime = mime;
		asset.size_bytes = static_cast<int64_t>(binary.size());
		asset.storage_provider = media_cfg.storage_provider;
		asset.storage_path = storage_path;
		asset.created_at_ms = NowMs();
		asset.deleted_at_ms = 0;
		asset.status = 1;
		if (!MysqlMgr::GetInstance()->InsertMediaAsset(asset)) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "save media metadata failed";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		std::filesystem::remove_all(temp_dir, ec);
		root["error"] = ErrorCodes::Success;
		root["media_key"] = media_key;
		root["media_type"] = media_type;
		root["file_name"] = file_name;
		root["mime"] = mime;
		root["size"] = static_cast<Json::Int64>(binary.size());
		root["url"] = std::string("/media/download?asset=") + media_key;
		beast::ostream(connection->_response.body()) << root.toStyledString();
		return true;
	});

	RegGet("/media/download", [](std::shared_ptr<HttpConnection> connection) {
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		const auto asset_it = connection->_get_params.find("asset");
		const auto file_it = connection->_get_params.find("file");
		const auto uid_it = connection->_get_params.find("uid");
		const auto token_it = connection->_get_params.find("token");
		if ((asset_it == connection->_get_params.end() && file_it == connection->_get_params.end()) ||
			uid_it == connection->_get_params.end() ||
			token_it == connection->_get_params.end()) {
			root["error"] = ErrorCodes::Error_Json;
			root["message"] = "missing media key or auth params";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const std::string media_key = (asset_it != connection->_get_params.end()) ? asset_it->second : "";
		const std::string legacy_file = (file_it != connection->_get_params.end()) ? file_it->second : "";
		const int uid = std::atoi(uid_it->second.c_str());
		const std::string token = token_it->second;
		if (uid <= 0 || !ValidateUserToken(uid, token)) {
			root["error"] = ErrorCodes::TokenInvalid;
			root["message"] = "token invalid";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		std::filesystem::path full_path;
		std::string content_type = "application/octet-stream";
		if (!media_key.empty()) {
			MediaAssetInfo asset;
			if (!MysqlMgr::GetInstance()->GetMediaAssetByKey(media_key, asset)
				|| asset.status != 1 || asset.deleted_at_ms > 0) {
				root["error"] = ErrorCodes::UidInvalid;
				root["message"] = "asset not found";
				beast::ostream(connection->_response.body()) << root.toStyledString();
				return true;
			}

			IMediaStorage& storage = MediaStorageFor(asset.storage_provider);
			if (!storage.ResolveReadPath(asset.storage_path, full_path) || !std::filesystem::exists(full_path)) {
				root["error"] = ErrorCodes::UidInvalid;
				root["message"] = "file not found";
				beast::ostream(connection->_response.body()) << root.toStyledString();
				return true;
			}
			content_type = GuessContentType(asset.origin_file_name, asset.mime);
		}
		else {
			if (!ResolveLegacyMediaPath(legacy_file, full_path) || !std::filesystem::exists(full_path)) {
				root["error"] = ErrorCodes::UidInvalid;
				root["message"] = "legacy file not found";
				beast::ostream(connection->_response.body()) << root.toStyledString();
				return true;
			}
			content_type = GuessContentType(full_path.filename().string(), "");
		}

		connection->_response.set(http::field::content_type, content_type);
		connection->SetFileResponse(full_path.string(), content_type);
		return true;
	});

	RegPost("/user_update_profile", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!src_root.isMember("uid") || !src_root.isMember("nick") ||
			!src_root.isMember("desc") || !src_root.isMember("icon")) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto uid = src_root["uid"].asInt();
		auto name = src_root.get("name", "").asString();
		auto nick = src_root["nick"].asString();
		auto desc = src_root["desc"].asString();
		auto icon = src_root["icon"].asString();
		if (uid <= 0 || nick.empty()) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		bool b_up = MysqlMgr::GetInstance()->UpdateUserProfile(uid, nick, desc, icon);
		if (!b_up) {
			root["error"] = ErrorCodes::ProfileUpFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto uid_str = std::to_string(uid);
		RedisMgr::GetInstance()->Del("ubaseinfo_" + uid_str);
		if (!name.empty()) {
			RedisMgr::GetInstance()->Del("nameinfo_" + name);
		}
		InvalidateLoginCacheByUid(uid);

		root["error"] = ErrorCodes::Success;
		root["uid"] = uid;
		root["name"] = name;
		root["nick"] = nick;
		root["desc"] = desc;
		root["icon"] = icon;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

    const auto handle_call_post = [](std::shared_ptr<HttpConnection> connection,
                                     const std::function<bool(const Json::Value&, Json::Value&, const std::string&)>& fn) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        if (!reader.parse(body_str, src_root)) {
            root["error"] = ErrorCodes::Error_Json;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
        fn(src_root, root, connection->_trace_id);
        if (!root.isMember("trace_id")) {
            root["trace_id"] = connection->_trace_id;
        }
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    };

    RegPost("/api/call/start", [handle_call_post](std::shared_ptr<HttpConnection> connection) {
        return handle_call_post(connection, [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            memolog::LogInfo("call.start.requested", "call start requested", {{"trace_id", trace_id}, {"module", "call"}});
            return CallService::GetInstance()->StartCall(src_root, root, trace_id);
        });
    });

    RegPost("/api/call/accept", [handle_call_post](std::shared_ptr<HttpConnection> connection) {
        return handle_call_post(connection, [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            return CallService::GetInstance()->AcceptCall(src_root, root, trace_id);
        });
    });

    RegPost("/api/call/reject", [handle_call_post](std::shared_ptr<HttpConnection> connection) {
        return handle_call_post(connection, [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            return CallService::GetInstance()->RejectCall(src_root, root, trace_id);
        });
    });

    RegPost("/api/call/cancel", [handle_call_post](std::shared_ptr<HttpConnection> connection) {
        return handle_call_post(connection, [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            return CallService::GetInstance()->CancelCall(src_root, root, trace_id);
        });
    });

    RegPost("/api/call/hangup", [handle_call_post](std::shared_ptr<HttpConnection> connection) {
        return handle_call_post(connection, [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            return CallService::GetInstance()->HangupCall(src_root, root, trace_id);
        });
    });

    RegGet("/api/call/token", [](std::shared_ptr<HttpConnection> connection) {
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        const int uid = std::atoi(connection->_get_params["uid"].c_str());
        const std::string token = connection->_get_params["token"];
        const std::string call_id = connection->_get_params["call_id"];
        const std::string role = connection->_get_params["role"];
        CallService::GetInstance()->GetToken(uid, token, call_id, role, root, connection->_trace_id);
        if (!root.isMember("trace_id")) {
            root["trace_id"] = connection->_trace_id;
        }
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

    RegPost("/api/call/token", [handle_call_post](std::shared_ptr<HttpConnection> connection) {
        return handle_call_post(connection, [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string call_id = src_root.get("call_id", "").asString();
            const std::string role = src_root.get("role", "").asString();
            return CallService::GetInstance()->GetToken(uid, token, call_id, role, root, trace_id);
        });
    });
}
void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {
	_post_handlers.insert(make_pair(url, handler));
}

LogicSystem::~LogicSystem() {

}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con) {
	if (_get_handlers.find(path) == _get_handlers.end()) {
		return false;
	}

	_get_handlers[path](con);
	return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		memolog::LogWarn("gate.http.post.not_found", "post route not found", { {"route", path} });
		return false;
	}

	memolog::TraceContext::SetTraceId(con ? con->_trace_id : "");
	memolog::LogInfo("gate.http.post.dispatch", "dispatch post route", { {"route", path} });
	_post_handlers[path](con);
	return true;
}
