#pragma once

#include "../GateServerCore/Http2MediaSupport.h"

namespace DrogonMediaSupport {
    using Http2MediaSupport::HandleUploadMediaInit;
    using Http2MediaSupport::HandleUploadMediaChunk;
    using Http2MediaSupport::HandleUploadMediaComplete;
    using Http2MediaSupport::HandleUploadMediaSimple;
    using Http2MediaSupport::HandleUploadMediaStatus;
    using Http2MediaSupport::HandleMediaDownloadInfo;
}  // namespace DrogonMediaSupport
