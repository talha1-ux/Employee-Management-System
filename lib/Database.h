#ifndef DATABASE_H
#define DATABASE_H

#include "Employee.h"
#include <vector>
#include <QString>
#include <functional>

struct FilterCriteria {
    QString nameQuery;
    QString department;
    QString designation;
    QString status;
    double  minSalary;
    double  maxSalary;
    double  minAttendance;
    QString gender;

    FilterCriteria()
        : minSalary(0), maxSalary(99999999), minAttendance(0) {}
};

struct DepartmentStats {
    QString dept;
    int     headCount;
    double  totalPayroll;
    double  avgSalary;
    double  avgAttendance;
};

class Database {
private:
    std::vector<Employee> repository;
    QString filename;

public:
    explicit Database(const QString& dbPath);

    void readDataFromFile();
    void writeDataToFile();

    void    insertRecord(const Employee& emp);
    bool    removeRecord(int id);
    bool    modifyRecord(int id, const Employee& updatedData);

    Employee*              lookupRecord(int id);
    std::vector<Employee>& pullAllRecords()     { return repository; }

    // --- Search & Filter (Static Polymorphism: overloads) ---
    std::vector<Employee> searchDatabase(const QString& query);
    std::vector<Employee> searchDatabase(double minSalary);
    std::vector<Employee> searchDatabase(const FilterCriteria& criteria);

    // --- Sorting ---
    void sortAlphabetically();
    void sortBySalaryRank();
    void sortByAttendance();
    void sortByDepartment();
    void sortById();

    // --- Analytics / Reports ---
    double  getTotalPayroll()     const;
    double  getAverageSalary()    const;
    double  getAverageAttendance()const;
    int     getActiveCount()      const;
    int     getOnLeaveCount()     const;
    std::vector<DepartmentStats> getDepartmentBreakdown() const;
    std::vector<std::pair<QString,int>> getDepartmentHeadcounts() const;

    // --- Unique helpers ---
    QStringList getUniqueDepartments() const;
    QStringList getUniqueDesignations() const;
    int          getNextId()            const;
};

#endif // DATABASE_H
