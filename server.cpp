#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "database.hpp"

class SQLCommand {
    MultiTable* mt;
    std::string sql;
    std::string result;
    const char* db_path;
public:
    SQLCommand(MultiTable* mt, const std::string& sql, const char* db_path)
        : mt(mt), sql(sql), db_path(db_path) {}
    void execute() {
        char buffer[65536];
        memset(buffer, 0, sizeof(buffer)); // ← ОБЯЗАТЕЛЬНО зануляем
        FILE* stream = fmemopen(buffer, sizeof(buffer), "w");
        int affected = execute_sql(mt, sql.c_str(), stream);
        fflush(stream);
        result = buffer;
        fclose(stream);
        if (affected >= 0) {
            if (affected == 0) {
                if (result.empty() || result.back() != '\n')
                    result += "OK\n";
                else
                    result += "OK\n";
            } else {
                result += "Affected rows: " + std::to_string(affected) + "\n";
            }
        }
        multitable_save(mt, db_path);
    }
    std::string getResult() const { return result; }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <db_file>\n";
        return 1;
    }
    int port = std::stoi(argv[1]);
    const char* db_file = argv[2];
    MultiTable* mt = multitable_load(db_file);
    if (!mt) mt = multitable_load(db_file);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);
    std::cout << "Server listening on port " << port << ", db file: " << db_file << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        char buffer[4096];
        ssize_t n = read(client_fd, buffer, sizeof(buffer)-1);
        if (n > 0) {
            buffer[n] = '\0';
            std::string sql(buffer);
            // удаляем возможный перевод строки
            if (!sql.empty() && sql.back() == '\n') sql.pop_back();
            SQLCommand cmd(mt, sql, db_file);
            cmd.execute();
            std::string result = cmd.getResult();
            send(client_fd, result.c_str(), result.size(), 0);
        }
        close(client_fd);
    }
    multitable_free(mt);
    close(server_fd);
    return 0;
}