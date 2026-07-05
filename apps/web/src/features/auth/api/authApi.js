import { ENDPOINTS } from "@/core/config/endpoints";
export function createAuthApi(http) {
    return {
        getVarifyCode: (email) => http.post(ENDPOINTS.getVarifyCode, { email }, { auth: false }),
        register: (params) => http.post(ENDPOINTS.register, params, { auth: false }),
        resetPassword: (email, code, newPassword) => http.post(ENDPOINTS.resetPwd, {
            email, varify_code: code, passwd: newPassword,
        }, { auth: false }),
    };
}
