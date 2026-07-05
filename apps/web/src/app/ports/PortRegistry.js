let _registry = null;
export function setPortRegistry(r) { _registry = r; }
export function getPortRegistry() {
    if (!_registry)
        throw new Error("PortRegistry not initialized");
    return _registry;
}
export function clearPortRegistry() { _registry = null; }
