#ifndef USER_H
#define USER_H

#include <QString>

// OOP Abstraction: Abstract interface enforcing structural layout rules
class User {
protected:
    QString username;
    QString passwordHash;
    QString activeRole;  // "admin", "hr", "viewer"
    QString fullName;
    QString email;
    QString lastLogin;

public:
    User(QString u, QString pw, QString r, QString fn = "", QString em = "")
        : username(u), passwordHash(pw), activeRole(r), fullName(fn), email(em) {}
    virtual ~User() {}

    // Pure Virtual Function - enforces implementation contract
    virtual QString getSystemAccessRights() const = 0;

    QString getUsername()    const { return username; }
    QString getPasswordHash()const { return passwordHash; }
    QString getActiveRole()  const { return activeRole; }
    QString getFullName()    const { return fullName; }
    QString getEmail()       const { return email; }
    QString getLastLogin()   const { return lastLogin; }
    void setLastLogin(const QString& dt) { lastLogin = dt; }

    bool validatePassword(const QString& pw) const { return passwordHash == pw; }
};

// ---- Concrete Role Implementations ----

class AdminUser : public User {
public:
    AdminUser(QString u, QString pw, QString fn = "System Administrator", QString em = "admin@company.com")
        : User(u, pw, "admin", fn, em) {}

    QString getSystemAccessRights() const override {
        return "FULL_ACCESS: read, write, delete, manage_users, generate_reports, export_data";
    }
};

class HRUser : public User {
public:
    HRUser(QString u, QString pw, QString fn = "HR Manager", QString em = "hr@company.com")
        : User(u, pw, "hr", fn, em) {}

    QString getSystemAccessRights() const override {
        return "HR_ACCESS: read, write, attendance, payroll, generate_reports";
    }
};

class ViewerUser : public User {
public:
    ViewerUser(QString u, QString pw, QString fn = "Viewer", QString em = "viewer@company.com")
        : User(u, pw, "viewer", fn, em) {}

    QString getSystemAccessRights() const override {
        return "READ_ONLY: read, view_reports";
    }
};

#endif // USER_H
