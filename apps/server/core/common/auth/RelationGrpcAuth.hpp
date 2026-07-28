#pragma once

#include <algorithm>
#include <string>
#include <string_view>

#include <grpcpp/client_context.h>
#include <grpcpp/server_context.h>
#include <sodium.h>

namespace memochat::auth
{
inline constexpr std::string_view RelationGrpcAuthMetadataKey()
{
    return "x-memochat-relation-auth";
}

inline constexpr std::size_t MinimumRelationGrpcAuthTokenBytes()
{
    return 32U;
}

inline bool IsStrongRelationGrpcAuthToken(std::string_view token)
{
    return token.size() >= MinimumRelationGrpcAuthTokenBytes() && std::ranges::all_of(token,
                                                                                      [](unsigned char value)
                                                                                      {
                                                                                          return value >= 0x21U &&
                                                                                                 value <= 0x7eU;
                                                                                      });
}

inline void InjectRelationGrpcAuth(grpc::ClientContext& context, const std::string& token)
{
    if (!token.empty())
    {
        context.AddMetadata(std::string(RelationGrpcAuthMetadataKey()), token);
    }
}

inline bool HasValidRelationGrpcAuth(const grpc::ServerContext& context, std::string_view expected_token)
{
    if (!IsStrongRelationGrpcAuthToken(expected_token))
    {
        return false;
    }

    const auto& metadata = context.client_metadata();
    const auto item = metadata.find(std::string(RelationGrpcAuthMetadataKey()));
    if (item == metadata.end())
    {
        return false;
    }

    const std::string_view supplied_token(item->second.data(), item->second.size());
    return supplied_token.size() == expected_token.size() &&
           sodium_memcmp(supplied_token.data(), expected_token.data(), expected_token.size()) == 0;
}
} // namespace memochat::auth
