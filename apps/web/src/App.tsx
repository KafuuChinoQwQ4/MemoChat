import { AppProviders } from "@/app/providers/AppProviders"
import { AppRouter } from "@/routes/AppRouter"

/** Root component: providers shell wrapping the router. No business logic here. */
export default function App() {
  return (
    <AppProviders>
      <AppRouter />
    </AppProviders>
  )
}
