/** Auth API — HTTP calls for register, verify code, reset password */
import type { HttpClient } from "@/core/network/http/HttpClient"
import { ENDPOINTS } from "@/core/config/endpoints"

export interface RegisterParams {
  email: string
  password: string
  code: string
  name: string
}

export interface RegisterResponse { error: number; uid?: number }
export interface VarifyCodeResponse { error: number }
export interface ResetPwdResponse { error: number }

export function createAuthApi(http: HttpClient) {
  return {
    getVarifyCode: (email: string) =>
      http.post<VarifyCodeResponse>(ENDPOINTS.getVarifyCode, { email }, { auth: false }),

    register: (params: RegisterParams) =>
      http.post<RegisterResponse>(ENDPOINTS.register, params, { auth: false }),

    resetPassword: (email: string, code: string, newPassword: string) =>
      http.post<ResetPwdResponse>(ENDPOINTS.resetPwd, {
        email, varify_code: code, passwd: newPassword,
      }, { auth: false }),
  }
}

export type AuthApi = ReturnType<typeof createAuthApi>
