CC     = g++
CFLAGS = -std=c++11 -Wall -Wextra -pedantic -O2
LDFLAGS =
TARGET = dbquery
OBJS   = database.o main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

database.o: database.cpp database.hpp
	$(CC) $(CFLAGS) -c -o $@ database.cpp

main.o: main.cpp database.hpp
	$(CC) $(CFLAGS) -c -o $@ main.cpp

test: $(TARGET)
	@echo "=== Test 1: CREATE TABLE and SELECT ==="
	@rm -f /tmp/db1.csv
	@./$(TARGET) /tmp/db1.csv "CREATE TABLE t1 (a INT, b TEXT);" /tmp/db1.csv
	@./$(TARGET) /tmp/db1.csv "SELECT * FROM t1;" > /tmp/out1.csv
	@printf "a,b\n" | diff - /tmp/out1.csv && echo "PASS" || echo "FAIL"

	@echo "=== Test 2: INSERT and SELECT ==="
	@./$(TARGET) /tmp/db1.csv "INSERT INTO t1 VALUES (1, 'hello');" /tmp/db2.csv
	@./$(TARGET) /tmp/db2.csv "SELECT * FROM t1;" > /tmp/out2.csv
	@printf "a,b\n1,hello\n" | diff - /tmp/out2.csv && echo "PASS" || echo "FAIL"

	@echo "=== Test 3: INSERT with another row ==="
	@./$(TARGET) /tmp/db2.csv "INSERT INTO t1 VALUES (2, 'world');" /tmp/db3.csv
	@./$(TARGET) /tmp/db3.csv "SELECT * FROM t1;" > /tmp/out3.csv
	@printf "a,b\n1,hello\n2,world\n" | diff - /tmp/out3.csv && echo "PASS" || echo "FAIL"

	@echo "=== Test 4: WHERE clause ==="
	@./$(TARGET) /tmp/db3.csv "SELECT a,b FROM t1 WHERE a > 1;" > /tmp/out4.csv
	@printf "a,b\n2,world\n" | diff - /tmp/out4.csv && echo "PASS" || echo "FAIL"

	@echo "=== Test 5: DELETE with WHERE ==="
	@./$(TARGET) /tmp/db3.csv "DELETE FROM t1 WHERE a = 1;" /tmp/db5.csv
	@./$(TARGET) /tmp/db5.csv "SELECT * FROM t1;" > /tmp/out5.csv
	@printf "a,b\n2,world\n" | diff - /tmp/out5.csv && echo "PASS" || echo "FAIL"

	@echo "=== Test 6: DROP TABLE and IF EXISTS ==="
	@./$(TARGET) /tmp/db5.csv "DROP TABLE IF EXISTS t1;" /tmp/db6.csv
	@./$(TARGET) /tmp/db6.csv "SELECT * FROM t1;" > /tmp/out6.csv 2>&1; \
	if grep -q "not found" /tmp/out6.csv; then echo "PASS (table gone)"; else echo "FAIL"; fi

	@echo "=== Test 7: CREATE with IF NOT EXISTS ==="
	@./$(TARGET) /tmp/db6.csv "CREATE TABLE IF NOT EXISTS t1 (x INT);" /tmp/db7.csv
	@./$(TARGET) /tmp/db7.csv "SELECT * FROM t1;" > /tmp/out7.csv
	@printf "x\n" | diff - /tmp/out7.csv && echo "PASS" || echo "FAIL"

	@echo "=== Test 8: INSERT and SELECT specific column ==="
	@./$(TARGET) /tmp/db7.csv "INSERT INTO t1 VALUES (100);" /tmp/db8.csv
	@./$(TARGET) /tmp/db8.csv "SELECT x FROM t1;" > /tmp/out8.csv
	@printf "x\n100\n" | diff - /tmp/out8.csv && echo "PASS" || echo "FAIL"

	@echo "=== Test 9: WHERE with string column ==="
	@./$(TARGET) example.csv "SELECT name,age FROM employees WHERE age > 28;" > /tmp/out9.csv
	@printf "name,age\nAlice,30\n" | diff - /tmp/out9.csv && echo "PASS" || echo "FAIL"

	@echo "=== Test 10: DELETE all rows ==="
	@./$(TARGET) /tmp/db3.csv "DELETE FROM t1;" /tmp/db10.csv
	@./$(TARGET) /tmp/db10.csv "SELECT * FROM t1;" > /tmp/out10.csv
	@printf "a,b\n" | diff - /tmp/out10.csv && echo "PASS" || echo "FAIL"

	@echo "=== Test 11: Complex WHERE with AND ==="
	@./$(TARGET) example.csv "SELECT id,name,age FROM employees WHERE age > 20 AND age < 40;" > /tmp/out11.csv
	@printf "id,name,age\n1,Alice,30\n2,Bob,25\n" | diff - /tmp/out11.csv && echo "PASS" || echo "FAIL"

	@echo "All tests done."
	rm -f /tmp/db*.csv /tmp/out*.csv

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all test clean