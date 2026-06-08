#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <QString>
#include <QDateTime>

class Employee {
private:
    int id;
    QString name;
    QString department;
    QString designation;
    QString email;
    QString phone;
    QString joinDate;
    QString status;       // "Active", "On Leave", "Resigned"
    QString gender;
    QString cnic;                   // Pakistani CNIC: XXXXX-XXXXXXX-X
    QString fingerprintTemplate;    // Hex/Base64 biometric template from hardware SDK
    QString faceIdTemplate;         // Face ID biometric template

    // Financial
    double basicSalary;
    double allowance;
    double deductions;

    // Attendance
    int presentCount;
    int absentCount;
    int leaveCount;

public:
    Employee();
    Employee(int id, const QString& name, const QString& dept, const QString& desig,
             double salary, double allow, double deduct,
             const QString& email = "", const QString& phone = "",
             const QString& joinDate = "", const QString& status = "Active",
             const QString& gender = "",
             const QString& cnic = "",
             const QString& fingerprintTemplate = "");

    virtual ~Employee() {}

    // --- Getters / Setters ---
    int     getId()          const { return id; }
    void    setId(int v)           { id = v; }

    QString getName()        const { return name; }
    void    setName(const QString& v) { name = v; }

    QString getDepartment()  const { return department; }
    void    setDepartment(const QString& v) { department = v; }

    QString getDesignation() const { return designation; }
    void    setDesignation(const QString& v) { designation = v; }

    QString getEmail()       const { return email; }
    void    setEmail(const QString& v) { email = v; }

    QString getPhone()       const { return phone; }
    void    setPhone(const QString& v) { phone = v; }

    QString getJoinDate()    const { return joinDate; }
    void    setJoinDate(const QString& v) { joinDate = v; }

    QString getStatus()      const { return status; }
    void    setStatus(const QString& v) { status = v; }

    QString getGender()      const { return gender; }
    void    setGender(const QString& v) { gender = v; }

    QString getCnic()        const { return cnic; }
    void    setCnic(const QString& v) { cnic = v; }

    // Fingerprint biometric template (empty = not enrolled)
    QString getFingerprintTemplate()  const { return fingerprintTemplate; }
    void    setFingerprintTemplate(const QString& v) { fingerprintTemplate = v; }
    bool    hasFingerprintEnrolled()  const { return !fingerprintTemplate.isEmpty(); }

    // Face ID biometric template (empty = not enrolled)
    QString getFaceIdTemplate()       const { return faceIdTemplate; }
    void    setFaceIdTemplate(const QString& v) { faceIdTemplate = v; }
    bool    hasFaceIdEnrolled()       const { return !faceIdTemplate.isEmpty(); }

    double  getBasicSalary() const { return basicSalary; }
    void    setBasicSalary(double v) { basicSalary = v; }

    double  getAllowance()   const { return allowance; }
    void    setAllowance(double v) { allowance = v; }

    double  getDeductions()  const { return deductions; }
    void    setDeductions(double v) { deductions = v; }

    // Attendance
    void logPresent()  { presentCount++; }
    void logAbsent()   { absentCount++; }
    void logLeave()    { leaveCount++; }
    int  getPresentDays()  const { return presentCount; }
    int  getAbsentDays()   const { return absentCount; }
    int  getLeaveDays()    const { return leaveCount; }
    int  getTotalAttendanceMetric() const { return presentCount + absentCount + leaveCount; }
    void setAttendanceMetrics(int p, int a, int l) { presentCount = p; absentCount = a; leaveCount = l; }

    // Computed
    virtual double  calculateNetPay()      const;
    virtual QString generateSlipPayload()  const;

    double getAttendancePercent() const {
        int total = getTotalAttendanceMetric();
        return total > 0 ? (presentCount * 100.0) / total : 100.0;
    }
};

#endif // EMPLOYEE_H
