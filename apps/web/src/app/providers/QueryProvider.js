import { jsx as _jsx } from "react/jsx-runtime";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
const queryClient = new QueryClient({
    defaultOptions: {
        queries: {
            retry: 1,
            staleTime: 30000,
            gcTime: 5 * 60000,
        },
    },
});
export function QueryProvider({ children }) {
    return _jsx(QueryClientProvider, { client: queryClient, children: children });
}
export { queryClient };
