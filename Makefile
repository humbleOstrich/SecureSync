CC     = g++
CFLAGS = -std=c++11 -Wall -Wextra -pedantic -O2 -fPIC
BUILD_DIR = build
LIBRARY = $(BUILD_DIR)/libdatabase.a
SERVER_TARGET = $(BUILD_DIR)/dbserver
CONSOLE_TARGET = $(BUILD_DIR)/dbquery
CLIENT_TARGET = qt_client/dbclient
OBJS   = $(BUILD_DIR)/database.o
SERVER_OBJ = $(BUILD_DIR)/server.o
CONSOLE_OBJ = $(BUILD_DIR)/main.o

all: $(LIBRARY) $(SERVER_TARGET) $(CONSOLE_TARGET) $(CLIENT_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIBRARY): $(BUILD_DIR) $(OBJS)
	ar rcs $@ $(OBJS)

$(SERVER_TARGET): $(BUILD_DIR) $(SERVER_OBJ) $(LIBRARY)
	$(CC) -o $@ $(SERVER_OBJ) -L$(BUILD_DIR) -ldatabase

$(CONSOLE_TARGET): $(BUILD_DIR) $(CONSOLE_OBJ) $(LIBRARY)
	$(CC) -o $@ $(CONSOLE_OBJ) -L$(BUILD_DIR) -ldatabase

$(CLIENT_TARGET):
	cd qt_client && (qmake6 || qmake) && $(MAKE)

$(BUILD_DIR)/database.o: database.cpp database.hpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ database.cpp

$(BUILD_DIR)/server.o: server.cpp database.hpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ server.cpp

$(BUILD_DIR)/main.o: main.cpp database.hpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ main.cpp

clean:
	rm -rf $(BUILD_DIR)
	rm -rf qt_client/Makefile qt_client/*.o qt_client/dbclient qt_client/.qmake.stash
	rm -rf /tmp/db_test mydb.db test_server.db test_gui.db

test: $(CONSOLE_TARGET)
	@./test.sh

test_server: $(SERVER_TARGET)
	@./test_server.sh

.PHONY: all clean test test_server