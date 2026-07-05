import { jsx as _jsx } from "react/jsx-runtime";
import { createContext, useContext, useRef } from "react";
import { getGateway } from "@/shared/gateway/ClientGateway";
const GatewayContext = createContext(null);
export function GatewayProvider({ children }) {
    const gatewayRef = useRef(getGateway());
    return (_jsx(GatewayContext.Provider, { value: gatewayRef.current, children: children }));
}
export function useGateway() {
    const gw = useContext(GatewayContext);
    if (!gw)
        throw new Error("useGateway must be used inside GatewayProvider");
    return gw;
}
