import { createContext, useContext, useRef, type ReactNode } from "react"
import { getGateway, type ClientGateway } from "@/shared/gateway/ClientGateway"

const GatewayContext = createContext<ClientGateway | null>(null)

export function GatewayProvider({ children }: { children: ReactNode }) {
  const gatewayRef = useRef<ClientGateway>(getGateway())
  return (
    <GatewayContext.Provider value={gatewayRef.current}>
      {children}
    </GatewayContext.Provider>
  )
}

export function useGateway(): ClientGateway {
  const gw = useContext(GatewayContext)
  if (!gw) throw new Error("useGateway must be used inside GatewayProvider")
  return gw
}
