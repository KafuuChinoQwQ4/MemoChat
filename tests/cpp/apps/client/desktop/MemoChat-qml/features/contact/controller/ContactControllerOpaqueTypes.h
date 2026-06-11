#ifndef CONTACT_CONTROLLER_OPAQUE_TYPES_H
#define CONTACT_CONTROLLER_OPAQUE_TYPES_H

#include <QMetaType>

class ApplyRequestModel;
class FriendListModel;
class SearchResultModel;

Q_DECLARE_OPAQUE_POINTER(ApplyRequestModel*)
Q_DECLARE_OPAQUE_POINTER(FriendListModel*)
Q_DECLARE_OPAQUE_POINTER(SearchResultModel*)

#endif // CONTACT_CONTROLLER_OPAQUE_TYPES_H
