import { useRef, type ReactNode } from "react"
import { getGateway, type ClientGateway } from "@/shared/gateway/ClientGateway"
import { GatewayContext } from "./gatewayContext"

export function GatewayProvider({ children }: { children: ReactNode }) {
  const gatewayRef = useRef<ClientGateway>(getGateway())
  return (
    <GatewayContext.Provider value={gatewayRef.current}>
      {children}
    </GatewayContext.Provider>
  )
}
