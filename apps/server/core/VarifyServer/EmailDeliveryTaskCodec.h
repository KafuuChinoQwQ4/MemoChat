#pragma once

#include <glaze/glaze.hpp>

#include <string>

namespace varifyservice
{

struct EmailDeliveryTaskPayload
{
    std::string email;
    std::string code;
    std::string trace_id;
    int retry_count = 0;
};

std::string SerializeEmailTask(const EmailDeliveryTaskPayload& task);
bool ParseEmailTask(const std::string& body, EmailDeliveryTaskPayload* out);

} // namespace varifyservice

template <> struct glz::meta<varifyservice::EmailDeliveryTaskPayload>
{
    using T = varifyservice::EmailDeliveryTaskPayload;
    static constexpr auto value =
        glz::object("email", &T::email, "code", &T::code, "trace_id", &T::trace_id, "retry_count", &T::retry_count);
};
