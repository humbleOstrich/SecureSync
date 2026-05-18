#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "sqlite3.h"
#include <string>
#include <vector>

struct User {
    int id;
    std::string name;
    std::string role;
};

class SecureSync {
private:
    sqlite3* db;
    std::string currentUserRole;

public:
    SecureSync(const std::string& db_name);
    ~SecureSync();

    // Системные методы
    bool initStructure();
    void setCurrentUserRole(const std::string& role) { currentUserRole = role; }

    // Бизнес-логика (с проверкой прав)
    bool addUser(int id, const std::string& name, const std::string& role);
    bool deleteUser(int id);
    std::vector<User> getAllUsers();
    
    // Аудит
    void logAction(const std::string& action);
    void showAuditLogs();
};

#endif