import { jsx as _jsx } from "react/jsx-runtime";
import { AppProviders } from "@/app/providers/AppProviders";
import { AppRouter } from "@/routes/AppRouter";
/** Root component: providers shell wrapping the router. No business logic here. */
export default function App() {
    return (_jsx(AppProviders, { children: _jsx(AppRouter, {}) }));
}
