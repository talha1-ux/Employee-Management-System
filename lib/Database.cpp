#include "Database.h"
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <QStringList>
#include <QSet>

Database::Database(const QString& dbPath) : filename(dbPath) {}

void Database::readDataFromFile() {
    repository.clear();
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList f = line.split("||");
        if (f.size() >= 13) {
            Employee emp(f[0].toInt(), f[1], f[2], f[3],
                         f[4].toDouble(), f[5].toDouble(), f[6].toDouble(),
                         f[7], f[8], f[9], f[10], f[11],
                         f.size() > 15 ? f[15] : "",   // CNIC
                         f.size() > 16 ? f[16] : "");  // fingerprintTemplate
            emp.setAttendanceMetrics(f[12].toInt(),
                                     f.size() > 13 ? f[13].toInt() : 0,
                                     f.size() > 14 ? f[14].toInt() : 0);
            if (f.size() > 17) emp.setFaceIdTemplate(f[17]);  // faceIdTemplate
            repository.push_back(emp);
        } else if (f.size() >= 10) {
            // Legacy format compatibility
            Employee emp(f[0].toInt(), f[1], f[2], f[3],
                         f[4].toDouble(), f[5].toDouble(), f[6].toDouble());
            emp.setAttendanceMetrics(f[7].toInt(), f[8].toInt(), f[9].toInt());
            repository.push_back(emp);
        }
    }
    file.close();
}

void Database::writeDataToFile() {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    for (const auto& emp : repository) {
        out << emp.getId()                  << "||"
            << emp.getName()                << "||"
            << emp.getDepartment()          << "||"
            << emp.getDesignation()         << "||"
            << emp.getBasicSalary()         << "||"
            << emp.getAllowance()           << "||"
            << emp.getDeductions()          << "||"
            << emp.getEmail()               << "||"
            << emp.getPhone()               << "||"
            << emp.getJoinDate()            << "||"
            << emp.getStatus()              << "||"
            << emp.getGender()              << "||"
            << emp.getPresentDays()         << "||"
            << emp.getAbsentDays()          << "||"
            << emp.getLeaveDays()           << "||"
            << emp.getCnic()                << "||"
            << emp.getFingerprintTemplate() << "||"
            << emp.getFaceIdTemplate()      << "\n";
    }
    file.close();
}

void Database::insertRecord(const Employee& emp) {
    repository.push_back(emp);
    writeDataToFile();
}

bool Database::removeRecord(int id) {
    auto sz = repository.size();
    repository.erase(
        std::remove_if(repository.begin(), repository.end(),
            [id](const Employee& e){ return e.getId() == id; }),
        repository.end());
    if (repository.size() != sz) { writeDataToFile(); return true; }
    return false;
}

bool Database::modifyRecord(int id, const Employee& updated) {
    for (auto& emp : repository) {
        if (emp.getId() == id) {
            emp = updated;
            writeDataToFile();
            return true;
        }
    }
    return false;
}

Employee* Database::lookupRecord(int id) {
    for (auto& emp : repository)
        if (emp.getId() == id) return &emp;
    return nullptr;
}

// Overload 1: name/dept text search
std::vector<Employee> Database::searchDatabase(const QString& query) {
    std::vector<Employee> out;
    for (const auto& emp : repository) {
        if (emp.getName().contains(query, Qt::CaseInsensitive)       ||
            emp.getDepartment().contains(query, Qt::CaseInsensitive) ||
            emp.getDesignation().contains(query, Qt::CaseInsensitive)||
            emp.getEmail().contains(query, Qt::CaseInsensitive)      ||
            QString::number(emp.getId()).contains(query))
        {
            out.push_back(emp);
        }
    }
    return out;
}

// Overload 2: minimum salary filter
std::vector<Employee> Database::searchDatabase(double minSalary) {
    std::vector<Employee> out;
    for (const auto& emp : repository)
        if (emp.calculateNetPay() >= minSalary) out.push_back(emp);
    return out;
}

// Overload 3: multi-criteria filter
std::vector<Employee> Database::searchDatabase(const FilterCriteria& c) {
    std::vector<Employee> out;
    for (const auto& emp : repository) {
        if (!c.nameQuery.isEmpty() &&
            !emp.getName().contains(c.nameQuery, Qt::CaseInsensitive) &&
            !emp.getDepartment().contains(c.nameQuery, Qt::CaseInsensitive))
            continue;
        if (!c.department.isEmpty() && emp.getDepartment() != c.department) continue;
        if (!c.designation.isEmpty() && emp.getDesignation() != c.designation) continue;
        if (!c.status.isEmpty() && emp.getStatus() != c.status) continue;
        if (!c.gender.isEmpty() && emp.getGender() != c.gender) continue;
        double net = emp.calculateNetPay();
        if (net < c.minSalary || net > c.maxSalary) continue;
        if (emp.getAttendancePercent() < c.minAttendance) continue;
        out.push_back(emp);
    }
    return out;
}

void Database::sortAlphabetically() {
    std::sort(repository.begin(), repository.end(),
        [](const Employee& a, const Employee& b){ return a.getName().toLower() < b.getName().toLower(); });
}

void Database::sortBySalaryRank() {
    std::sort(repository.begin(), repository.end(),
        [](const Employee& a, const Employee& b){ return a.calculateNetPay() > b.calculateNetPay(); });
}

void Database::sortByAttendance() {
    std::sort(repository.begin(), repository.end(),
        [](const Employee& a, const Employee& b){ return a.getAttendancePercent() > b.getAttendancePercent(); });
}

void Database::sortByDepartment() {
    std::sort(repository.begin(), repository.end(),
        [](const Employee& a, const Employee& b){ return a.getDepartment().toLower() < b.getDepartment().toLower(); });
}

void Database::sortById() {
    std::sort(repository.begin(), repository.end(),
        [](const Employee& a, const Employee& b){ return a.getId() < b.getId(); });
}

double Database::getTotalPayroll() const {
    double sum = 0;
    for (const auto& e : repository) sum += e.calculateNetPay();
    return sum;
}

double Database::getAverageSalary() const {
    if (repository.empty()) return 0;
    return getTotalPayroll() / repository.size();
}

double Database::getAverageAttendance() const {
    if (repository.empty()) return 100;
    double sum = 0;
    for (const auto& e : repository) sum += e.getAttendancePercent();
    return sum / repository.size();
}

int Database::getActiveCount() const {
    int c = 0;
    for (const auto& e : repository) if (e.getStatus() == "Active") c++;
    return c;
}

int Database::getOnLeaveCount() const {
    int c = 0;
    for (const auto& e : repository) if (e.getStatus() == "On Leave") c++;
    return c;
}

std::vector<DepartmentStats> Database::getDepartmentBreakdown() const {
    QStringList depts;
    for (const auto& e : repository) {
        if (!depts.contains(e.getDepartment())) depts << e.getDepartment();
    }

    std::vector<DepartmentStats> result;
    for (const auto& dept : depts) {
        DepartmentStats ds;
        ds.dept = dept;
        ds.headCount = 0;
        ds.totalPayroll = 0;
        ds.avgAttendance = 0;
        for (const auto& e : repository) {
            if (e.getDepartment() == dept) {
                ds.headCount++;
                ds.totalPayroll += e.calculateNetPay();
                ds.avgAttendance += e.getAttendancePercent();
            }
        }
        ds.avgSalary = ds.headCount > 0 ? ds.totalPayroll / ds.headCount : 0;
        ds.avgAttendance = ds.headCount > 0 ? ds.avgAttendance / ds.headCount : 0;
        result.push_back(ds);
    }
    return result;
}

std::vector<std::pair<QString,int>> Database::getDepartmentHeadcounts() const {
    std::vector<std::pair<QString,int>> out;
    for (const auto& ds : getDepartmentBreakdown())
        out.push_back({ds.dept, ds.headCount});
    return out;
}

QStringList Database::getUniqueDepartments() const {
    QStringList list;
    for (const auto& e : repository)
        if (!list.contains(e.getDepartment())) list << e.getDepartment();
    list.sort();
    return list;
}

QStringList Database::getUniqueDesignations() const {
    QStringList list;
    for (const auto& e : repository)
        if (!list.contains(e.getDesignation())) list << e.getDesignation();
    list.sort();
    return list;
}

int Database::getNextId() const {
    int maxId = 80500;
    for (const auto& e : repository)
        if (e.getId() > maxId) maxId = e.getId();
    return maxId + 1;
}
