#include "AppController.h"

#include "AppCoordinators.h"
#include "AppHttpEventRouter.h"
#include "httpmgr.h"

#include <utility>

void AppController::bindAppHttpSignals()
{
    _http_event_router = std::make_unique<AppHttpEventRouter>(
        *_session_coordinator,
        _features.profileController,
        [this]()
        {
            emit settingsStatusChanged();
        },
        this);
    connect(_gateway.httpMgr().get(),
            &HttpMgr::sig_login_mod_finish,
            _http_event_router.get(),
            &AppHttpEventRouter::onLoginHttpFinished);
    connect(_gateway.httpMgr().get(),
            &HttpMgr::sig_reg_mod_finish,
            _http_event_router.get(),
            &AppHttpEventRouter::onRegisterHttpFinished);
    connect(_gateway.httpMgr().get(),
            &HttpMgr::sig_reset_mod_finish,
            _http_event_router.get(),
            &AppHttpEventRouter::onResetHttpFinished);
    connect(_gateway.httpMgr().get(),
            &HttpMgr::sig_settings_mod_finish,
            _http_event_router.get(),
            &AppHttpEventRouter::onSettingsHttpFinished);
    connect(_gateway.httpMgr().get(),
            &HttpMgr::sig_call_mod_finish,
            this,
            [this](ReqId id, QString res, ErrorCodes err)
            {
                _call_coordinator->onCallHttpFinished(id, std::move(res), err);
            });
}
