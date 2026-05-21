#include <iostream>
#include <string>
#include <cstdlib>
#include "database.hpp"

static bool quiet = false;

static void print_affected(int n) {
    if (!quiet && n != 0)
        std::cout << "Affected rows: " << n << std::endl;
}

static void repl(MultiTable *mt, const std::string &filename) {
    std::string line;
    std::cout << "SecureSQL> ";
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            std::cout << "SecureSQL> ";
            continue;
        }
        if (line == "exit" || line == "quit") break;

        int affected = execute_sql(mt, line.c_str(), stdout);
        if (affected >= 0) print_affected(affected);
        else std::cerr << "Error executing statement" << std::endl;

        if (multitable_save(mt, filename.c_str()) != 0)
            std::cerr << "Warning: failed to save to " << filename << std::endl;

        std::cout << "SecureSQL> ";
    }
}

int main(int argc, char *argv[]) {
    int opt_idx = 1;
    if (argc > 1 && std::string(argv[1]) == "-q") {
        quiet = true;
        opt_idx = 2;
    }

    if (argc < opt_idx + 1 || argc > opt_idx + 3) {
        std::cerr << "Usage: " << argv[0] << " [-q] <data_file> [sql_command] [output_csv]\n"
                  << "  -q  quiet mode (suppress 'Affected rows')\n";
        return 1;
    }

    std::string filename = argv[opt_idx];
    MultiTable *mt = multitable_load(filename.c_str());
    if (!mt) {
        std::cerr << "Failed to load database from " << filename << std::endl;
        return 1;
    }

    if (argc == opt_idx + 1) {
        repl(mt, filename);
        if (multitable_save(mt, filename.c_str()) != 0)
            std::cerr << "Failed to save to " << filename << std::endl;
    } else {
        std::string sql = argv[opt_idx + 1];
        int affected = execute_sql(mt, sql.c_str(), stdout);
        if (affected < 0) {
            std::cerr << "Execution failed" << std::endl;
            multitable_free(mt);
            return 1;
        }
        print_affected(affected);
        const char *outfile = (argc == opt_idx + 3) ? argv[opt_idx + 2] : filename.c_str();
        if (multitable_save(mt, outfile) != 0) {
            std::cerr << "Failed to save to " << outfile << std::endl;
            multitable_free(mt);
            return 1;
        }
    }

    multitable_free(mt);
    return 0;
}