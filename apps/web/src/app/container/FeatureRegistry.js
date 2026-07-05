const _registry = new Map();
export const FeatureRegistry = {
    register(name, lifecycle) {
        _registry.set(name, lifecycle);
    },
    async bootstrapAll() {
        for (const [, lc] of _registry) {
            await lc.bootstrap?.();
        }
    },
    teardownAll() {
        for (const [, lc] of _registry) {
            lc.teardown?.();
        }
    },
    clear() {
        _registry.clear();
    },
};
