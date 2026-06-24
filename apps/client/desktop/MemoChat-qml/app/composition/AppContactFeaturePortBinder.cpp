#include "AppPortRegistry.h"
#include "usermgr.h"

void AppPortRegistry::bindContactFeaturePorts()
{
    _features.contactController.setBootstrapPort(
        ContactBootstrapPort{[this]()
                             {
                                 ensureChatListInitialized();
                             },
                             [this]()
                             {
                                 return _gateway.userMgr()->GetConListPerPage();
                             },
                             [this]()
                             {
                                 _gateway.userMgr()->UpdateContactLoadedCount();
                             },
                             [this]()
                             {
                                 return _gateway.userMgr()->IsLoadConFin();
                             }});

    _features.contactController.setApplyBootstrapPort(
        ContactApplyBootstrapPort{[this]()
                                  {
                                      return _gateway.userMgr()->GetApplyListSnapshot();
                                  }});

    _features.contactController.setCommandPort(ContactCommandPort{[this]()
                                                                  {
                                                                      jumpChatWithCurrentContact();
                                                                  },
                                                                  [this]()
                                                                  {
                                                                      requestRelationBootstrap();
                                                                  }});
}
