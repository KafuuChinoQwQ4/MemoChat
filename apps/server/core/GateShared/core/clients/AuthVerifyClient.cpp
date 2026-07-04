#include "AuthVerifyClient.hpp"

#include "VerifyGrpcClient.hpp"
#include "const.hpp"

namespace memochat::gate::core
{

AuthVerifyClient& AuthVerifyClient::Instance()
{
    static AuthVerifyClient instance;
    return instance;
}

VerifyCodeRequestResult AuthVerifyClient::RequestVerifyCode(const std::string& email)
{
    auto rsp = ::VerifyGrpcClient::GetInstance()->GetVarifyCode(email);

    VerifyCodeRequestResult result{ErrorCodes::RPCFailed, email};
    result.error = rsp.error();
    return result;
}

} // namespace memochat::gate::core
