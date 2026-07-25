import { createContext, useContext } from "react"
import type { ClientGateway } from "@/shared/gateway/ClientGateway"

export const GatewayContext = createContext<ClientGateway | null>(null)

export function useGateway(): ClientGateway {
  const gateway = useContext(GatewayContext)
  if (!gateway) throw new Error("useGateway must be used inside GatewayProvider")
  return gateway
}
