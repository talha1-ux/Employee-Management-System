#ifndef MANAGER_H
#define MANAGER_H

#include "Employee.h"

// OOP Inheritance + Polymorphism
class ExecutiveManager : public Employee {
private:
    double managementBonus;
    int    directReports;

public:
    ExecutiveManager(int id, const QString& name, const QString& dept, const QString& desig,
                     double salary, double allow, double deduct, double bonus,
                     int reports = 0,
                     const QString& email = "", const QString& phone = "",
                     const QString& joinDate = "", const QString& status = "Active",
                     const QString& gender = "")
        : Employee(id, name, dept, desig, salary, allow, deduct, email, phone, joinDate, status, gender),
          managementBonus(bonus), directReports(reports) {}

    double getBonus()       const { return managementBonus; }
    int    getDirectReports()const { return directReports; }

    double calculateNetPay() const override {
        return Employee::calculateNetPay() + managementBonus;
    }

    QString generateSlipPayload() const override {
        return Employee::generateSlipPayload()
            + QString("\n  Management Bonus  : Rs. %1\n"
                      "  Direct Reports    : %2\n")
              .arg(managementBonus, 0, 'f', 2)
              .arg(directReports);
    }
};

#endif // MANAGER_H
