import { getContainer } from "./DIContainer"

/** Hook to access the DI container from React components (rare — prefer port hooks) */
export function useContainer() {
  return getContainer()
}
