#pragma once

#include <string>

namespace CertUtil {

bool GenerateSelfSignedCertPem(const std::string& crt_path, const std::string& key_path);

bool GenerateSelfSignedCertPfx(const std::string& pfx_path,
                               const std::string& crt_path,
                               const std::string& key_path,
                               const std::string& password);

}  // namespace CertUtil
