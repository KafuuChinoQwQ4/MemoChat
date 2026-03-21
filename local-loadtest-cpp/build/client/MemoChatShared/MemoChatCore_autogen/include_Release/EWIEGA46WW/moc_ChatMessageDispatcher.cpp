/****************************************************************************
** Meta object code from reading C++ file 'ChatMessageDispatcher.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../../client/MemoChatShared/ChatMessageDispatcher.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ChatMessageDispatcher.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN21ChatMessageDispatcherE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN21ChatMessageDispatcherE = QtMocHelpers::stringData(
    "ChatMessageDispatcher",
    "sig_swich_chatdlg",
    "",
    "sig_load_apply_list",
    "json_array",
    "sig_login_failed",
    "sig_user_search",
    "std::shared_ptr<SearchInfo>",
    "sig_friend_apply",
    "std::shared_ptr<AddFriendApply>",
    "sig_add_auth_friend",
    "std::shared_ptr<AuthInfo>",
    "sig_auth_rsp",
    "std::shared_ptr<AuthRsp>",
    "sig_text_chat_msg",
    "std::shared_ptr<TextChatMsg>",
    "msg",
    "sig_group_list_updated",
    "sig_group_invite",
    "groupId",
    "groupCode",
    "groupName",
    "operatorUid",
    "sig_group_apply",
    "applicantUid",
    "applicantUserId",
    "reason",
    "sig_group_member_changed",
    "payload",
    "sig_group_chat_msg",
    "std::shared_ptr<GroupChatMsg>",
    "sig_group_rsp",
    "ReqId",
    "reqId",
    "error",
    "sig_relation_bootstrap_updated",
    "sig_dialog_list_rsp",
    "sig_private_history_rsp",
    "sig_private_msg_changed",
    "sig_private_read_ack",
    "sig_message_status",
    "sig_call_event",
    "sig_heartbeat_ack",
    "ackAtMs",
    "sig_notify_offline"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN21ChatMessageDispatcherE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      23,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      23,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  152,    2, 0x06,    1 /* Public */,
       3,    1,  153,    2, 0x06,    2 /* Public */,
       5,    1,  156,    2, 0x06,    4 /* Public */,
       6,    1,  159,    2, 0x06,    6 /* Public */,
       8,    1,  162,    2, 0x06,    8 /* Public */,
      10,    1,  165,    2, 0x06,   10 /* Public */,
      12,    1,  168,    2, 0x06,   12 /* Public */,
      14,    1,  171,    2, 0x06,   14 /* Public */,
      17,    0,  174,    2, 0x06,   16 /* Public */,
      18,    4,  175,    2, 0x06,   17 /* Public */,
      23,    4,  184,    2, 0x06,   22 /* Public */,
      27,    1,  193,    2, 0x06,   27 /* Public */,
      29,    1,  196,    2, 0x06,   29 /* Public */,
      31,    3,  199,    2, 0x06,   31 /* Public */,
      35,    0,  206,    2, 0x06,   35 /* Public */,
      36,    1,  207,    2, 0x06,   36 /* Public */,
      37,    1,  210,    2, 0x06,   38 /* Public */,
      38,    1,  213,    2, 0x06,   40 /* Public */,
      39,    1,  216,    2, 0x06,   42 /* Public */,
      40,    1,  219,    2, 0x06,   44 /* Public */,
      41,    1,  222,    2, 0x06,   46 /* Public */,
      42,    1,  225,    2, 0x06,   48 /* Public */,
      44,    0,  228,    2, 0x06,   50 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QJsonArray,    4,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, 0x80000000 | 7,    2,
    QMetaType::Void, 0x80000000 | 9,    2,
    QMetaType::Void, 0x80000000 | 11,    2,
    QMetaType::Void, 0x80000000 | 13,    2,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, QMetaType::Int,   19,   20,   21,   22,
    QMetaType::Void, QMetaType::LongLong, QMetaType::Int, QMetaType::QString, QMetaType::QString,   19,   24,   25,   26,
    QMetaType::Void, QMetaType::QJsonObject,   28,
    QMetaType::Void, 0x80000000 | 30,   16,
    QMetaType::Void, 0x80000000 | 32, QMetaType::Int, QMetaType::QJsonObject,   33,   34,   28,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QJsonObject,   28,
    QMetaType::Void, QMetaType::QJsonObject,   28,
    QMetaType::Void, QMetaType::QJsonObject,   28,
    QMetaType::Void, QMetaType::QJsonObject,   28,
    QMetaType::Void, QMetaType::QJsonObject,   28,
    QMetaType::Void, QMetaType::QJsonObject,   28,
    QMetaType::Void, QMetaType::LongLong,   43,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject ChatMessageDispatcher::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN21ChatMessageDispatcherE.offsetsAndSizes,
    qt_meta_data_ZN21ChatMessageDispatcherE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN21ChatMessageDispatcherE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ChatMessageDispatcher, std::true_type>,
        // method 'sig_swich_chatdlg'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sig_load_apply_list'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonArray, std::false_type>,
        // method 'sig_login_failed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'sig_user_search'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<SearchInfo>, std::false_type>,
        // method 'sig_friend_apply'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<AddFriendApply>, std::false_type>,
        // method 'sig_add_auth_friend'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<AuthInfo>, std::false_type>,
        // method 'sig_auth_rsp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<AuthRsp>, std::false_type>,
        // method 'sig_text_chat_msg'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<TextChatMsg>, std::false_type>,
        // method 'sig_group_list_updated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sig_group_invite'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'sig_group_apply'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'sig_group_member_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'sig_group_chat_msg'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<GroupChatMsg>, std::false_type>,
        // method 'sig_group_rsp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ReqId, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'sig_relation_bootstrap_updated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sig_dialog_list_rsp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'sig_private_history_rsp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'sig_private_msg_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'sig_private_read_ack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'sig_message_status'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'sig_call_event'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'sig_heartbeat_ack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'sig_notify_offline'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void ChatMessageDispatcher::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ChatMessageDispatcher *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->sig_swich_chatdlg(); break;
        case 1: _t->sig_load_apply_list((*reinterpret_cast< std::add_pointer_t<QJsonArray>>(_a[1]))); break;
        case 2: _t->sig_login_failed((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->sig_user_search((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<SearchInfo>>>(_a[1]))); break;
        case 4: _t->sig_friend_apply((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<AddFriendApply>>>(_a[1]))); break;
        case 5: _t->sig_add_auth_friend((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<AuthInfo>>>(_a[1]))); break;
        case 6: _t->sig_auth_rsp((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<AuthRsp>>>(_a[1]))); break;
        case 7: _t->sig_text_chat_msg((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<TextChatMsg>>>(_a[1]))); break;
        case 8: _t->sig_group_list_updated(); break;
        case 9: _t->sig_group_invite((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 10: _t->sig_group_apply((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 11: _t->sig_group_member_changed((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 12: _t->sig_group_chat_msg((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<GroupChatMsg>>>(_a[1]))); break;
        case 13: _t->sig_group_rsp((*reinterpret_cast< std::add_pointer_t<ReqId>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[3]))); break;
        case 14: _t->sig_relation_bootstrap_updated(); break;
        case 15: _t->sig_dialog_list_rsp((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 16: _t->sig_private_history_rsp((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 17: _t->sig_private_msg_changed((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 18: _t->sig_private_read_ack((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 19: _t->sig_message_status((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 20: _t->sig_call_event((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 21: _t->sig_heartbeat_ack((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 22: _t->sig_notify_offline(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (ChatMessageDispatcher::*)();
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_swich_chatdlg; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(QJsonArray );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_load_apply_list; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(int );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_login_failed; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(std::shared_ptr<SearchInfo> );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_user_search; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(std::shared_ptr<AddFriendApply> );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_friend_apply; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(std::shared_ptr<AuthInfo> );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_add_auth_friend; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(std::shared_ptr<AuthRsp> );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_auth_rsp; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(std::shared_ptr<TextChatMsg> );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_text_chat_msg; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)();
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_group_list_updated; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(qint64 , QString , QString , int );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_group_invite; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(qint64 , int , QString , QString );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_group_apply; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(QJsonObject );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_group_member_changed; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(std::shared_ptr<GroupChatMsg> );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_group_chat_msg; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(ReqId , int , QJsonObject );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_group_rsp; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)();
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_relation_bootstrap_updated; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(QJsonObject );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_dialog_list_rsp; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(QJsonObject );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_private_history_rsp; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 16;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(QJsonObject );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_private_msg_changed; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 17;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(QJsonObject );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_private_read_ack; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 18;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(QJsonObject );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_message_status; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 19;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(QJsonObject );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_call_event; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 20;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)(qint64 );
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_heartbeat_ack; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 21;
                return;
            }
        }
        {
            using _q_method_type = void (ChatMessageDispatcher::*)();
            if (_q_method_type _q_method = &ChatMessageDispatcher::sig_notify_offline; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 22;
                return;
            }
        }
    }
}

const QMetaObject *ChatMessageDispatcher::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChatMessageDispatcher::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN21ChatMessageDispatcherE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ChatMessageDispatcher::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 23;
    }
    return _id;
}

// SIGNAL 0
void ChatMessageDispatcher::sig_swich_chatdlg()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ChatMessageDispatcher::sig_load_apply_list(QJsonArray _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ChatMessageDispatcher::sig_login_failed(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ChatMessageDispatcher::sig_user_search(std::shared_ptr<SearchInfo> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ChatMessageDispatcher::sig_friend_apply(std::shared_ptr<AddFriendApply> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ChatMessageDispatcher::sig_add_auth_friend(std::shared_ptr<AuthInfo> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void ChatMessageDispatcher::sig_auth_rsp(std::shared_ptr<AuthRsp> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void ChatMessageDispatcher::sig_text_chat_msg(std::shared_ptr<TextChatMsg> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void ChatMessageDispatcher::sig_group_list_updated()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void ChatMessageDispatcher::sig_group_invite(qint64 _t1, QString _t2, QString _t3, int _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void ChatMessageDispatcher::sig_group_apply(qint64 _t1, int _t2, QString _t3, QString _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void ChatMessageDispatcher::sig_group_member_changed(QJsonObject _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void ChatMessageDispatcher::sig_group_chat_msg(std::shared_ptr<GroupChatMsg> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void ChatMessageDispatcher::sig_group_rsp(ReqId _t1, int _t2, QJsonObject _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void ChatMessageDispatcher::sig_relation_bootstrap_updated()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void ChatMessageDispatcher::sig_dialog_list_rsp(QJsonObject _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}

// SIGNAL 16
void ChatMessageDispatcher::sig_private_history_rsp(QJsonObject _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void ChatMessageDispatcher::sig_private_msg_changed(QJsonObject _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void ChatMessageDispatcher::sig_private_read_ack(QJsonObject _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}

// SIGNAL 19
void ChatMessageDispatcher::sig_message_status(QJsonObject _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 19, _a);
}

// SIGNAL 20
void ChatMessageDispatcher::sig_call_event(QJsonObject _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 20, _a);
}

// SIGNAL 21
void ChatMessageDispatcher::sig_heartbeat_ack(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 21, _a);
}

// SIGNAL 22
void ChatMessageDispatcher::sig_notify_offline()
{
    QMetaObject::activate(this, &staticMetaObject, 22, nullptr);
}
QT_WARNING_POP
