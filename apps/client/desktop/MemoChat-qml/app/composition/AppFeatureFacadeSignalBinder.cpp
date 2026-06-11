#include "AppController.h"

void AppController::bindAppFeatureFacadeSignals()
{
    connect(&_features.contactController,
            &ContactController::contactPaneChanged,
            this,
            &AppController::contactPaneChanged);
    connect(&_features.contactController,
            &ContactController::currentContactChanged,
            this,
            &AppController::currentContactChanged);
    connect(&_features.contactController,
            &ContactController::searchPendingChanged,
            this,
            &AppController::searchPendingChanged);
    connect(&_features.contactController,
            &ContactController::searchStatusChanged,
            this,
            &AppController::searchStatusChanged);
    connect(&_features.contactController,
            &ContactController::authStatusChanged,
            this,
            &AppController::authStatusChanged);
    connect(&_features.contactController,
            &ContactController::pendingApplyChanged,
            this,
            &AppController::pendingApplyChanged);
    connect(&_features.contactController,
            &ContactController::contactLoadingMoreChanged,
            this,
            &AppController::contactLoadingMoreChanged);
    connect(&_features.contactController,
            &ContactController::canLoadMoreContactsChanged,
            this,
            &AppController::canLoadMoreContactsChanged);
    connect(&_features.contactController,
            &ContactController::contactsReadyChanged,
            this,
            &AppController::lazyBootstrapStateChanged);
    connect(&_features.contactController,
            &ContactController::applyReadyChanged,
            this,
            &AppController::lazyBootstrapStateChanged);
    connect(&_features.groupController, &GroupController::groupCreated, this, &AppController::groupCreated);
}
