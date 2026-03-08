#pragma once

#include <grpcpp/grpcpp.h>

namespace memolog {

void InjectGrpcTraceMetadata(grpc::ClientContext& context);
void BindGrpcTraceContext(grpc::ServerContext* context);

} // namespace memolog
