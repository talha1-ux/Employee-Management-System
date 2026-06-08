#include "Employee.h"

Employee::Employee()
    : id(0), name(""), department(""), designation(""),
      email(""), phone(""), joinDate(""), status("Active"), gender(""),
      cnic(""), fingerprintTemplate(""), faceIdTemplate(""),
      basicSalary(0), allowance(0), deductions(0),
      presentCount(0), absentCount(0), leaveCount(0) {}

Employee::Employee(int id, const QString& name, const QString& dept, const QString& desig,
                   double salary, double allow, double deduct,
                   const QString& email, const QString& phone,
                   const QString& joinDate, const QString& status,
                   const QString& gender,
                   const QString& cnic,
                   const QString& fingerprintTemplate)
    : id(id), name(name), department(dept), designation(desig),
      email(email), phone(phone), joinDate(joinDate), status(status), gender(gender),
      cnic(cnic), fingerprintTemplate(fingerprintTemplate), faceIdTemplate(""),
      basicSalary(salary), allowance(allow), deductions(deduct),
      presentCount(0), absentCount(0), leaveCount(0) {}

double Employee::calculateNetPay() const {
    return (basicSalary + allowance) - deductions;
}

QString Employee::generateSlipPayload() const {
    return QString(
        "╔══════════════════════════════════════╗\n"
        "     ENTERPRISE PAY SLIP DOCUMENT\n"
        "╚══════════════════════════════════════╝\n\n"
        "  Employee ID   : %1\n"
        "  Full Name     : %2\n"
        "  CNIC          : %3\n"
        "  Department    : %4\n"
        "  Designation   : %5\n"
        "  Email         : %6\n"
        "  Phone         : %7\n"
        "  Join Date     : %8\n"
        "  Status        : %9\n\n"
        "──────────────────────────────────────\n"
        "  SALARY BREAKDOWN\n"
        "──────────────────────────────────────\n"
        "  Basic Salary  : Rs. %10\n"
        "  Allowances    : Rs. %11\n"
        "  Deductions    : Rs. %12\n"
        "──────────────────────────────────────\n"
        "  NET SALARY    : Rs. %13\n"
        "══════════════════════════════════════\n\n"
        "  Attendance Summary\n"
        "  Present: %14 | Absent: %15 | Leave: %16\n"
        "  Attendance Rate: %17%\n"
        "  Fingerprint Enrolled: %18\n"
        "  Face ID Enrolled:     %19\n"
    )
    .arg(id).arg(name)
    .arg(cnic.isEmpty() ? "Not Provided" : cnic)
    .arg(department).arg(designation)
    .arg(email).arg(phone).arg(joinDate).arg(status)
    .arg(basicSalary, 0, 'f', 2)
    .arg(allowance,   0, 'f', 2)
    .arg(deductions,  0, 'f', 2)
    .arg(calculateNetPay(), 0, 'f', 2)
    .arg(presentCount).arg(absentCount).arg(leaveCount)
    .arg(getAttendancePercent(), 0, 'f', 1)
    .arg(hasFingerprintEnrolled() ? "Yes" : "No")
    .arg(hasFaceIdEnrolled() ? "Yes" : "No");
}
