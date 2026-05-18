#include "database.hpp"
#include <iostream>

void printMenu() {
    std::cout << 
	    "\t1. Показать пользователей\n"
	    "\t2. Добавить пользователя (Admin only)\n"
	    "\t3. Журнал аудита\n"
	    "\t4. Сменить роль\n"
	    "\t0. Выход\n> ";
}

int main() {
    try {
        SecureSync app("system.db");
        app.initStructure();
        app.setCurrentUserRole("guest"); // По умолчанию заходим как гость

        int choice = -1;
        while (choice != 0) {
            printMenu();
            std::cin >> choice;

            if (choice == 1) {
                for (const auto& u : app.getAllUsers()) 
                    std::cout << u.id << " | " << u.name << " (" << u.role << ")\n";
            } 
            else if (choice == 2) {
                int id; std::string name, role;
                std::cout << "ID Name Role: ";
                std::cin >> id >> name >> role;
                if (app.addUser(id, name, role)) std::cout << "Успех!\n";
            } 
            else if (choice == 3) {
                app.showAuditLogs();
            }
            else if (choice == 4) {
                std::string newRole;
                std::cout << "Введите роль (admin/guest): ";
                std::cin >> newRole;
                app.setCurrentUserRole(newRole);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
