#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
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

	RegPost("/upload_media", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			root["error"] = ErrorCodes::Error_Json;
			root["message"] = "invalid json";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		if (!src_root.isMember("uid") || !src_root.isMember("file_name") || !src_root.isMember("data_base64")) {
			root["error"] = ErrorCodes::Error_Json;
			root["message"] = "missing fields";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const int uid = src_root["uid"].asInt();
		const std::string file_name = SanitizeFileName(src_root["file_name"].asString());
		const std::string mime = src_root.get("mime", "").asString();
		const std::string media_type = src_root.get("media_type", "").asString();
		const std::string encoded = src_root["data_base64"].asString();
		if (uid <= 0 || encoded.empty()) {
			root["error"] = ErrorCodes::Error_Json;
			root["message"] = "invalid media payload";
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
		if (binary.empty() || binary.size() > 8 * 1024 * 1024) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "file too large or empty";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		std::filesystem::path upload_dir = std::filesystem::current_path() / "uploads";
		std::error_code ec;
		std::filesystem::create_directories(upload_dir, ec);
		if (ec) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "create upload dir failed";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}

		const auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());
		const std::string stored_name = std::to_string(uid) + "_" + uuid + "_" + file_name;
		std::filesystem::path full_path = upload_dir / stored_name;
		std::ofstream ofs(full_path, std::ios::binary);
		if (!ofs.is_open()) {
			root["error"] = ErrorCodes::MediaUploadFailed;
			root["message"] = "open file failed";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return true;
		}
		ofs.write(binary.data(), static_cast<std::streamsize>(binary.size()));
		ofs.close();

		root["error"] = ErrorCodes::Success;
		root["media_type"] = media_type;
		root["file_name"] = file_name;
		root["mime"] = GuessContentType(file_name, mime);
		root["url"] = std::string("/media/download?file=") + stored_name;
		beast::ostream(connection->_response.body()) << root.toStyledString();
		return true;
	});

	RegGet("/media/download", [](std::shared_ptr<HttpConnection> connection) {
		Json::Value err_root;
		const auto file_it = connection->_get_params.find("file");
		if (file_it == connection->_get_params.end()) {
			err_root["error"] = ErrorCodes::Error_Json;
			err_root["message"] = "missing file";
			connection->_response.set(http::field::content_type, "text/json");
			beast::ostream(connection->_response.body()) << err_root.toStyledString();
			return true;
		}

		std::string stored_name = file_it->second;
		if (stored_name.empty() || stored_name.find("..") != std::string::npos
			|| stored_name.find('/') != std::string::npos || stored_name.find('\\') != std::string::npos) {
			err_root["error"] = ErrorCodes::Error_Json;
			err_root["message"] = "invalid file";
			connection->_response.set(http::field::content_type, "text/json");
			beast::ostream(connection->_response.body()) << err_root.toStyledString();
			return true;
		}

		std::filesystem::path full_path = std::filesystem::current_path() / "uploads" / stored_name;
		if (!std::filesystem::exists(full_path)) {
			err_root["error"] = ErrorCodes::UidInvalid;
			err_root["message"] = "file not found";
			connection->_response.set(http::field::content_type, "text/json");
			beast::ostream(connection->_response.body()) << err_root.toStyledString();
			return true;
		}

		std::ifstream ifs(full_path, std::ios::binary);
		if (!ifs.is_open()) {
			err_root["error"] = ErrorCodes::MediaUploadFailed;
			err_root["message"] = "open file failed";
			connection->_response.set(http::field::content_type, "text/json");
			beast::ostream(connection->_response.body()) << err_root.toStyledString();
			return true;
		}

		std::string binary((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		ifs.close();
		connection->_response.set(http::field::content_type, GuessContentType(stored_name, ""));
		auto body_stream = beast::ostream(connection->_response.body());
		body_stream.write(binary.data(), static_cast<std::streamsize>(binary.size()));
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
