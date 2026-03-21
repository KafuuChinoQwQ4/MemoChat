/****************************************************************************
** Meta object code from reading C++ file 'CallSessionModel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../../client/MemoChat-qml/CallSessionModel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CallSessionModel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16CallSessionModelE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN16CallSessionModelE = QtMocHelpers::stringData(
    "CallSessionModel",
    "changed",
    "",
    "visible",
    "incoming",
    "active",
    "tokenReady",
    "callId",
    "callType",
    "peerName",
    "peerIcon",
    "stateText",
    "roomName",
    "livekitUrl",
    "mediaLaunchJson",
    "mediaStatusText",
    "elapsedSeconds",
    "muted",
    "cameraEnabled"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN16CallSessionModelE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
      16,   21, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   20,    2, 0x06,   17 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
       3, QMetaType::Bool, 0x00015001, uint(0), 0,
       4, QMetaType::Bool, 0x00015001, uint(0), 0,
       5, QMetaType::Bool, 0x00015001, uint(0), 0,
       6, QMetaType::Bool, 0x00015001, uint(0), 0,
       7, QMetaType::QString, 0x00015001, uint(0), 0,
       8, QMetaType::QString, 0x00015001, uint(0), 0,
       9, QMetaType::QString, 0x00015001, uint(0), 0,
      10, QMetaType::QString, 0x00015001, uint(0), 0,
      11, QMetaType::QString, 0x00015001, uint(0), 0,
      12, QMetaType::QString, 0x00015001, uint(0), 0,
      13, QMetaType::QString, 0x00015001, uint(0), 0,
      14, QMetaType::QString, 0x00015001, uint(0), 0,
      15, QMetaType::QString, 0x00015001, uint(0), 0,
      16, QMetaType::Int, 0x00015001, uint(0), 0,
      17, QMetaType::Bool, 0x00015001, uint(0), 0,
      18, QMetaType::Bool, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject CallSessionModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN16CallSessionModelE.offsetsAndSizes,
    qt_meta_data_ZN16CallSessionModelE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN16CallSessionModelE_t,
        // property 'visible'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'incoming'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'active'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'tokenReady'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'callId'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'callType'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'peerName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'peerIcon'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'stateText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'roomName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'livekitUrl'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'mediaLaunchJson'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'mediaStatusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'elapsedSeconds'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'muted'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'cameraEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CallSessionModel, std::true_type>,
        // method 'changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void CallSessionModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CallSessionModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->changed(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (CallSessionModel::*)();
            if (_q_method_type _q_method = &CallSessionModel::changed; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->visible(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->incoming(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->active(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->tokenReady(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->callId(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->callType(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->peerName(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->peerIcon(); break;
        case 8: *reinterpret_cast< QString*>(_v) = _t->stateText(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->roomName(); break;
        case 10: *reinterpret_cast< QString*>(_v) = _t->livekitUrl(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->mediaLaunchJson(); break;
        case 12: *reinterpret_cast< QString*>(_v) = _t->mediaStatusText(); break;
        case 13: *reinterpret_cast< int*>(_v) = _t->elapsedSeconds(); break;
        case 14: *reinterpret_cast< bool*>(_v) = _t->muted(); break;
        case 15: *reinterpret_cast< bool*>(_v) = _t->cameraEnabled(); break;
        default: break;
        }
    }
}

const QMetaObject *CallSessionModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CallSessionModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN16CallSessionModelE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CallSessionModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void CallSessionModel::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
