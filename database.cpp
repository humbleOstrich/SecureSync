#include "database.hpp"
#include <iostream>

SecureSync::SecureSync(const std::string& db_name) {
    if (sqlite3_open(db_name.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Can't open database");
    }
}

SecureSync::~SecureSync() {
    sqlite3_close(db);
}

bool SecureSync::initStructure() {
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS users (id INT PRIMARY KEY, name TEXT, role TEXT);"
        "CREATE TABLE IF NOT EXISTS audit_log (id INTEGER PRIMARY KEY, action TEXT, time DATETIME DEFAULT CURRENT_TIMESTAMP);";
    return sqlite3_exec(db, sql, 0, 0, 0) == SQLITE_OK;
}

bool SecureSync::addUser(int id, const std::string& name, const std::string& role) {
    // ПРОВЕРКА ПРАВ (RBAC)
    if (currentUserRole != "admin") {
        std::cerr << "[Access Denied] Only admins can add users.\n";
        return false;
    }

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO users (id, name, role) VALUES (?, ?, ?);";
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success) logAction("Added user: " + name);
    return success;
}

std::vector<User> SecureSync::getAllUsers() {
    std::vector<User> results;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT * FROM users;", -1, &stmt, 0);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back({
            sqlite3_column_int(stmt, 0),
            (const char*)sqlite3_column_text(stmt, 1),
            (const char*)sqlite3_column_text(stmt, 2)
        });
    }
    sqlite3_finalize(stmt);
    return results;
}

void SecureSync::logAction(const std::string& action) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "INSERT INTO audit_log (action) VALUES (?);", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, action.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void SecureSync::showAuditLogs() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT * FROM audit_log ORDER BY time DESC;", -1, &stmt, 0);
    std::cout << "\n--- AUDIT LOG ---\n";
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::cout << "[" << sqlite3_column_text(stmt, 2) << "] " << sqlite3_column_text(stmt, 1) << "\n";
    }
    sqlite3_finalize(stmt);
}