/**
 * FeatureRegistry — mirrors AppFeatureRegistry.
 * Tracks which features have been initialized so bootstrap and teardown
 * can call them in order without each feature importing the others.
 */
export type FeatureLifecycle = {
  bootstrap?: () => Promise<void>
  teardown?: () => void
}

const _registry = new Map<string, FeatureLifecycle>()

export const FeatureRegistry = {
  register(name: string, lifecycle: FeatureLifecycle) {
    _registry.set(name, lifecycle)
  },
  async bootstrapAll() {
    for (const [, lc] of _registry) {
      await lc.bootstrap?.()
    }
  },
  teardownAll() {
    for (const [, lc] of _registry) {
      lc.teardown?.()
    }
  },
  clear() {
    _registry.clear()
  },
}
