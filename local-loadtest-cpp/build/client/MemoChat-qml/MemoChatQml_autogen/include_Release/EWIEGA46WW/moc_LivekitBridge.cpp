/****************************************************************************
** Meta object code from reading C++ file 'LivekitBridge.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../../client/MemoChat-qml/LivekitBridge.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LivekitBridge.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN13LivekitBridgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN13LivekitBridgeE = QtMocHelpers::stringData(
    "LivekitBridge",
    "stateChanged",
    "",
    "joinRequested",
    "wsUrl",
    "token",
    "metadataJson",
    "micToggleRequested",
    "cameraToggleRequested",
    "leaveRequested",
    "roomJoined",
    "remoteTrackReady",
    "roomDisconnected",
    "reason",
    "recoverable",
    "permissionError",
    "deviceType",
    "message",
    "mediaError",
    "reconnecting",
    "bridgeLog",
    "setPageReady",
    "ready",
    "toggleMic",
    "toggleCamera",
    "leaveRoom",
    "reportRoomJoined",
    "reportRemoteTrackReady",
    "reportRoomDisconnected",
    "reportPermissionError",
    "reportMediaError",
    "reportReconnecting",
    "reportLog",
    "roomPageUrl",
    "embeddedEnabled"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN13LivekitBridgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      23,   14, // methods
       2,  211, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  152,    2, 0x06,    3 /* Public */,
       3,    3,  153,    2, 0x06,    4 /* Public */,
       7,    0,  160,    2, 0x06,    8 /* Public */,
       8,    0,  161,    2, 0x06,    9 /* Public */,
       9,    0,  162,    2, 0x06,   10 /* Public */,
      10,    0,  163,    2, 0x06,   11 /* Public */,
      11,    0,  164,    2, 0x06,   12 /* Public */,
      12,    2,  165,    2, 0x06,   13 /* Public */,
      15,    2,  170,    2, 0x06,   16 /* Public */,
      18,    1,  175,    2, 0x06,   19 /* Public */,
      19,    1,  178,    2, 0x06,   21 /* Public */,
      20,    1,  181,    2, 0x06,   23 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      21,    1,  184,    2, 0x02,   25 /* Public */,
      23,    0,  187,    2, 0x02,   27 /* Public */,
      24,    0,  188,    2, 0x02,   28 /* Public */,
      25,    0,  189,    2, 0x02,   29 /* Public */,
      26,    0,  190,    2, 0x02,   30 /* Public */,
      27,    0,  191,    2, 0x02,   31 /* Public */,
      28,    2,  192,    2, 0x02,   32 /* Public */,
      29,    2,  197,    2, 0x02,   35 /* Public */,
      30,    1,  202,    2, 0x02,   38 /* Public */,
      31,    1,  205,    2, 0x02,   40 /* Public */,
      32,    1,  208,    2, 0x02,   42 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,    4,    5,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   13,   14,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   16,   17,
    QMetaType::Void, QMetaType::QString,   17,
    QMetaType::Void, QMetaType::QString,   17,
    QMetaType::Void, QMetaType::QString,   17,

 // methods: parameters
    QMetaType::Void, QMetaType::Bool,   22,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   13,   14,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   16,   17,
    QMetaType::Void, QMetaType::QString,   17,
    QMetaType::Void, QMetaType::QString,   17,
    QMetaType::Void, QMetaType::QString,   17,

 // properties: name, type, flags, notifyId, revision
      33, QMetaType::QString, 0x00015001, uint(0), 0,
      34, QMetaType::Bool, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject LivekitBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN13LivekitBridgeE.offsetsAndSizes,
    qt_meta_data_ZN13LivekitBridgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN13LivekitBridgeE_t,
        // property 'roomPageUrl'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'embeddedEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<LivekitBridge, std::true_type>,
        // method 'stateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'joinRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'micToggleRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cameraToggleRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'leaveRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'roomJoined'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'remoteTrackReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'roomDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'permissionError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'mediaError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'reconnecting'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'bridgeLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setPageReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'toggleMic'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleCamera'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'leaveRoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'reportRoomJoined'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'reportRemoteTrackReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'reportRoomDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'reportPermissionError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'reportMediaError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'reportReconnecting'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'reportLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void LivekitBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<LivekitBridge *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->stateChanged(); break;
        case 1: _t->joinRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 2: _t->micToggleRequested(); break;
        case 3: _t->cameraToggleRequested(); break;
        case 4: _t->leaveRequested(); break;
        case 5: _t->roomJoined(); break;
        case 6: _t->remoteTrackReady(); break;
        case 7: _t->roomDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 8: _t->permissionError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 9: _t->mediaError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->reconnecting((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->bridgeLog((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->setPageReady((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 13: _t->toggleMic(); break;
        case 14: _t->toggleCamera(); break;
        case 15: _t->leaveRoom(); break;
        case 16: _t->reportRoomJoined(); break;
        case 17: _t->reportRemoteTrackReady(); break;
        case 18: _t->reportRoomDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 19: _t->reportPermissionError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 20: _t->reportMediaError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 21: _t->reportReconnecting((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 22: _t->reportLog((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (LivekitBridge::*)();
            if (_q_method_type _q_method = &LivekitBridge::stateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)(const QString & , const QString & , const QString & );
            if (_q_method_type _q_method = &LivekitBridge::joinRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)();
            if (_q_method_type _q_method = &LivekitBridge::micToggleRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)();
            if (_q_method_type _q_method = &LivekitBridge::cameraToggleRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)();
            if (_q_method_type _q_method = &LivekitBridge::leaveRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)();
            if (_q_method_type _q_method = &LivekitBridge::roomJoined; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)();
            if (_q_method_type _q_method = &LivekitBridge::remoteTrackReady; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)(const QString & , bool );
            if (_q_method_type _q_method = &LivekitBridge::roomDisconnected; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)(const QString & , const QString & );
            if (_q_method_type _q_method = &LivekitBridge::permissionError; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)(const QString & );
            if (_q_method_type _q_method = &LivekitBridge::mediaError; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)(const QString & );
            if (_q_method_type _q_method = &LivekitBridge::reconnecting; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (LivekitBridge::*)(const QString & );
            if (_q_method_type _q_method = &LivekitBridge::bridgeLog; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->roomPageUrl(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->embeddedEnabled(); break;
        default: break;
        }
    }
}

const QMetaObject *LivekitBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LivekitBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN13LivekitBridgeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int LivekitBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void LivekitBridge::stateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void LivekitBridge::joinRequested(const QString & _t1, const QString & _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void LivekitBridge::micToggleRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void LivekitBridge::cameraToggleRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void LivekitBridge::leaveRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void LivekitBridge::roomJoined()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void LivekitBridge::remoteTrackReady()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void LivekitBridge::roomDisconnected(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void LivekitBridge::permissionError(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void LivekitBridge::mediaError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void LivekitBridge::reconnecting(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void LivekitBridge::bridgeLog(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}
QT_WARNING_POP
