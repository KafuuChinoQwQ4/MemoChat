#pragma once

#include <string>

namespace CertUtil {

bool GenerateSelfSignedCertPem(const std::string& crt_path, const std::string& key_path);

}  // namespace CertUtil
