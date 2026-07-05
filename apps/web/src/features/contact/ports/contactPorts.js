let _ports = null;
export function setContactPorts(p) { _ports = p; }
export function getContactPorts() {
    if (!_ports)
        throw new Error("ContactPorts not initialized");
    return _ports;
}
