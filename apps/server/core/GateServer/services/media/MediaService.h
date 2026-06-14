#pragma once

#include "routing/GateRequest.h"
#include "routing/GateResponse.h"

namespace memochat::gate::services::media
{

class MediaService
{
public:
    static MediaService& Instance();

    bool HandleUploadMediaInit(const memochat::gate::routing::GateRequest& request,
                               memochat::gate::routing::GateResponse& response);
    bool HandleUploadMediaChunk(const memochat::gate::routing::GateRequest& request,
                                memochat::gate::routing::GateResponse& response);
    bool HandleUploadMediaStatus(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response);
    bool HandleUploadMediaComplete(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response);
    bool HandleUploadMediaSimple(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response);
    bool HandleMediaDownload(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response);

private:
    MediaService() = default;
};

} // namespace memochat::gate::services::media
