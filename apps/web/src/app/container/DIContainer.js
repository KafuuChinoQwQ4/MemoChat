let _container = null;
export function buildContainer(gateway) {
    _container = { gateway };
    return _container;
}
export function getContainer() {
    if (!_container)
        throw new Error("DI container not built yet — call buildContainer first");
    return _container;
}
export function teardownContainer() {
    _container = null;
}
