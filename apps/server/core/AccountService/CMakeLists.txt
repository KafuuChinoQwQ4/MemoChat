add_executable(AccountServer
    app/AccountServer.cpp
)
memochat_configure_gate_focused_service(AccountServer DOMAINS GateAccountDomain)
