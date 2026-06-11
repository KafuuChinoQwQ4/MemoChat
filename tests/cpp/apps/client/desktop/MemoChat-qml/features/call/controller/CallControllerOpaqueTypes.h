#ifndef CALL_CONTROLLER_OPAQUE_TYPES_H
#define CALL_CONTROLLER_OPAQUE_TYPES_H

#include <QMetaType>

class CallSessionModel;
class LivekitBridge;

Q_DECLARE_OPAQUE_POINTER(CallSessionModel*)
Q_DECLARE_OPAQUE_POINTER(LivekitBridge*)

#endif // CALL_CONTROLLER_OPAQUE_TYPES_H
