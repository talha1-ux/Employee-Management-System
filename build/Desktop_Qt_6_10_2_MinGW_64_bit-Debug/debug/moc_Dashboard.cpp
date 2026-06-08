/****************************************************************************
** Meta object code from reading C++ file 'Dashboard.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../gui/Dashboard.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Dashboard.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
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
struct qt_meta_tag_ZN9DashboardE_t {};
} // unnamed namespace

template <> constexpr inline auto Dashboard::qt_create_metaobjectdata<qt_meta_tag_ZN9DashboardE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Dashboard",
        "executionAddWorkflow",
        "",
        "executionUpdateWorkflow",
        "executionDeleteWorkflow",
        "executionAttendanceWorkflow",
        "executionPaySlipWorkflow",
        "executionExportWorkflow",
        "executionLiveSearch",
        "text",
        "executionAdvancedFilter",
        "executionSortChanged",
        "idx",
        "onNavEmployees",
        "onNavReports",
        "onNavAdmin",
        "onNavLogout",
        "onGenerateFullReport",
        "onGenerateDeptReport",
        "onGenerateAttendanceReport",
        "onGeneratePayrollReport",
        "onExportReport",
        "onAddUser",
        "onRemoveUser",
        "onChangePassword",
        "onFingerprintScan",
        "onPrintEmployeeList",
        "onPrintReport",
        "onExportReportPDF"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'executionAddWorkflow'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'executionUpdateWorkflow'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'executionDeleteWorkflow'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'executionAttendanceWorkflow'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'executionPaySlipWorkflow'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'executionExportWorkflow'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'executionLiveSearch'
        QtMocHelpers::SlotData<void(const QString &)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
        // Slot 'executionAdvancedFilter'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'executionSortChanged'
        QtMocHelpers::SlotData<void(int)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 12 },
        }}),
        // Slot 'onNavEmployees'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNavReports'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNavAdmin'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNavLogout'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onGenerateFullReport'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onGenerateDeptReport'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onGenerateAttendanceReport'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onGeneratePayrollReport'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExportReport'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAddUser'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemoveUser'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onChangePassword'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFingerprintScan'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPrintEmployeeList'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPrintReport'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExportReportPDF'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Dashboard, qt_meta_tag_ZN9DashboardE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Dashboard::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9DashboardE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9DashboardE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9DashboardE_t>.metaTypes,
    nullptr
} };

void Dashboard::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Dashboard *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->executionAddWorkflow(); break;
        case 1: _t->executionUpdateWorkflow(); break;
        case 2: _t->executionDeleteWorkflow(); break;
        case 3: _t->executionAttendanceWorkflow(); break;
        case 4: _t->executionPaySlipWorkflow(); break;
        case 5: _t->executionExportWorkflow(); break;
        case 6: _t->executionLiveSearch((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->executionAdvancedFilter(); break;
        case 8: _t->executionSortChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->onNavEmployees(); break;
        case 10: _t->onNavReports(); break;
        case 11: _t->onNavAdmin(); break;
        case 12: _t->onNavLogout(); break;
        case 13: _t->onGenerateFullReport(); break;
        case 14: _t->onGenerateDeptReport(); break;
        case 15: _t->onGenerateAttendanceReport(); break;
        case 16: _t->onGeneratePayrollReport(); break;
        case 17: _t->onExportReport(); break;
        case 18: _t->onAddUser(); break;
        case 19: _t->onRemoveUser(); break;
        case 20: _t->onChangePassword(); break;
        case 21: _t->onFingerprintScan(); break;
        case 22: _t->onPrintEmployeeList(); break;
        case 23: _t->onPrintReport(); break;
        case 24: _t->onExportReportPDF(); break;
        default: ;
        }
    }
}

const QMetaObject *Dashboard::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Dashboard::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9DashboardE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int Dashboard::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 25)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 25;
    }
    return _id;
}
QT_WARNING_POP
