#include "AuthVerifyClient.h"

#include "VerifyGrpcClient.h"
#include "const.h"

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
