import { jsx as _jsx } from "react/jsx-runtime";
import { GatewayProvider } from "./GatewayProvider";
import { QueryProvider } from "./QueryProvider";
import { ThemeProvider } from "./ThemeProvider";
export function AppProviders({ children }) {
    return (_jsx(GatewayProvider, { children: _jsx(QueryProvider, { children: _jsx(ThemeProvider, { children: children }) }) }));
}
