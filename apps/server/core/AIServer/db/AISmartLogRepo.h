#pragma once
#include <string>
#include <memory>

class AISmartLogRepo {
public:
    explicit AISmartLogRepo();
    ~AISmartLogRepo();

    void LogSmartUsage(int32_t uid, const std::string& feature_type,
                       int input_tokens, int output_tokens,
                       const std::string& model_name);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
