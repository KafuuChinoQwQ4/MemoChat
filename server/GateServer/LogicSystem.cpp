#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"
#include "MediaStorage.h"
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

namespace {
const char* kMinClientVersion = "2.0.0";

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
	int max_image_bytes = 20 * 1024 * 1024;
	int max_file_bytes = 200 * 1024 * 1024;
	int chunk_size_bytes = 1024 * 1024;
	int session_expire_sec = 86400;
	std::string storage_provider = "local";
};

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
	cfg.max_image_bytes = ParseConfigInt(media["MaxImageMB"], 20) * 1024 * 1024;
	cfg.max_file_bytes = ParseConfigInt(media["MaxFileMB"], 200) * 1024 * 1024;
	cfg.chunk_size_bytes = ParseConfigInt(media["ChunkSizeKB"], 1024) * 1024;
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

		bool pwd_valid = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
		if (!pwd_valid) {
			memolog::LogWarn("gate.user_login.failed", "password invalid",
				{ {"email", email}, {"error_code", std::to_string(ErrorCodes::PasswdInvalid)} });
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}


		auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
		if (reply.error()) {
			memolog::LogWarn("gate.user_login.failed", "get chat server rpc failed",
				{ {"uid", std::to_string(userInfo.uid)}, {"error_code", std::to_string(reply.error())} });
			root["error"] = ErrorCodes::RPCFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		memolog::TraceContext::SetUid(std::to_string(userInfo.uid));
		root["error"] = 0;
		root["email"] = email;
		root["uid"] = userInfo.uid;
		root["user_id"] = userInfo.user_id;
		root["token"] = reply.token();
		root["host"] = reply.host();
		root["port"] = reply.port();
		memolog::LogInfo("gate.user_login", "user login succeeded",
			{
				{"uid", std::to_string(userInfo.uid)},
				{"route", "/user_login"},
				{"chat_host", reply.host()},
				{"chat_port", reply.port()}
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
