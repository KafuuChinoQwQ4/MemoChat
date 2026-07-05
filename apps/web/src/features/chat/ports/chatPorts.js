let _ports = null;
export function setChatPorts(ports) { _ports = ports; }
export function getChatPorts() {
    if (!_ports)
        throw new Error("ChatPorts not initialized");
    return _ports;
}
