#ifndef USERDATABASE_H
#define USERDATABASE_H

#include "User.h"
#include <vector>
#include <memory>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

class UserDatabase {
private:
    std::vector<std::unique_ptr<User>> users;
    QString filename;

    void seedDefaultUsers() {
        // Default credentials seeded on first run
        users.push_back(std::make_unique<AdminUser>("admin", "admin123", "Ahmed Raza", "admin@company.com"));
        users.push_back(std::make_unique<HRUser>("hr", "hr123", "Sara Khan", "hr@company.com"));
        users.push_back(std::make_unique<ViewerUser>("viewer", "viewer123", "Ali Hassan", "viewer@company.com"));
        saveToFile();
    }

public:
    explicit UserDatabase(const QString& file) : filename(file) {
        loadFromFile();
    }

    void loadFromFile() {
        users.clear();
        QFile f(filename);
        if (!f.exists()) {
            seedDefaultUsers();
            return;
        }
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            seedDefaultUsers();
            return;
        }
        QTextStream in(&f);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            QStringList parts = line.split("||");
            if (parts.size() < 5) continue;
            QString role = parts[2];
            if (role == "admin")
                users.push_back(std::make_unique<AdminUser>(parts[0], parts[1], parts[3], parts[4]));
            else if (role == "hr")
                users.push_back(std::make_unique<HRUser>(parts[0], parts[1], parts[3], parts[4]));
            else
                users.push_back(std::make_unique<ViewerUser>(parts[0], parts[1], parts[3], parts[4]));
        }
        f.close();
        if (users.empty()) seedDefaultUsers();
    }

    void saveToFile() {
        QFile f(filename);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream out(&f);
        for (const auto& u : users) {
            out << u->getUsername() << "||"
                << u->getPasswordHash() << "||"
                << u->getActiveRole() << "||"
                << u->getFullName() << "||"
                << u->getEmail() << "\n";
        }
        f.close();
    }

    // Returns role string if valid, empty if not
    QString authenticate(const QString& username, const QString& password) {
        for (const auto& u : users) {
            if (u->getUsername() == username && u->validatePassword(password)) {
                return u->getActiveRole();
            }
        }
        return "";
    }

    User* findUser(const QString& username) {
        for (const auto& u : users)
            if (u->getUsername() == username) return u.get();
        return nullptr;
    }

    const std::vector<std::unique_ptr<User>>& getAllUsers() const { return users; }

    bool addUser(const QString& username, const QString& password, const QString& role,
                 const QString& fullName, const QString& email) {
        // Check duplicate
        for (const auto& u : users)
            if (u->getUsername() == username) return false;

        if (role == "admin")
            users.push_back(std::make_unique<AdminUser>(username, password, fullName, email));
        else if (role == "hr")
            users.push_back(std::make_unique<HRUser>(username, password, fullName, email));
        else
            users.push_back(std::make_unique<ViewerUser>(username, password, fullName, email));

        saveToFile();
        return true;
    }

    bool removeUser(const QString& username) {
        auto it = std::remove_if(users.begin(), users.end(),
            [&](const std::unique_ptr<User>& u){ return u->getUsername() == username; });
        if (it == users.end()) return false;
        users.erase(it, users.end());
        saveToFile();
        return true;
    }

    bool changePassword(const QString& username, const QString& newPassword) {
        // Re-create user with new password same role
        User* existing = findUser(username);
        if (!existing) return false;
        QString role = existing->getActiveRole();
        QString fn   = existing->getFullName();
        QString em   = existing->getEmail();
        removeUser(username);
        addUser(username, newPassword, role, fn, em);
        return true;
    }
};

#endif // USERDATABASE_H
