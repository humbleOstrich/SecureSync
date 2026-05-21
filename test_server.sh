#!/bin/bash

set -e

PORT=54323
DB_FILE="test_server.db"
SERVER_CMD="./build/dbserver"

rm -f "$DB_FILE"
rm -f /tmp/server_output.log

$SERVER_CMD $PORT "$DB_FILE" > /tmp/server_output.log 2>&1 &
SERVER_PID=$!
sleep 1

send_sql() {
    echo "$1" | nc -w 2 localhost $PORT
}

echo "Тестирование сервера:"

RESULT=$(send_sql "CREATE TABLE users (id INT, name TEXT);")
if echo "$RESULT" | grep -q "OK"; then
    echo "  PASS: CREATE TABLE"
else
    echo "  FAIL: CREATE TABLE (got: $RESULT)"
    kill $SERVER_PID
    exit 1
fi

RESULT=$(send_sql "INSERT INTO users VALUES (1, 'Alice');")
if echo "$RESULT" | grep -q "Affected rows: 1"; then
    echo "  PASS: INSERT"
else
    echo "  FAIL: INSERT"
    kill $SERVER_PID
    exit 1
fi

RESULT=$(send_sql "SELECT * FROM users;")
EXPECTED="id,name
1,Alice"
if echo "$RESULT" | grep -q "$EXPECTED"; then
    echo "  PASS: SELECT"
else
    echo "  FAIL: SELECT"
    kill $SERVER_PID
    exit 1
fi

RESULT=$(send_sql "UPDATE users SET name = 'Bob' WHERE id = 1;")
if echo "$RESULT" | grep -q "Affected rows: 1"; then
    echo "  PASS: UPDATE"
else
    echo "  FAIL: UPDATE"
    kill $SERVER_PID
    exit 1
fi

RESULT=$(send_sql "DELETE FROM users WHERE id = 1;")
if echo "$RESULT" | grep -q "Affected rows: 1"; then
    echo "  PASS: DELETE"
else
    echo "  FAIL: DELETE"
    kill $SERVER_PID
    exit 1
fi

RESULT=$(send_sql "DROP TABLE users;")
if echo "$RESULT" | grep -q "OK"; then
    echo "  PASS: DROP TABLE"
else
    echo "  FAIL: DROP TABLE"
    kill $SERVER_PID
    exit 1
fi

kill $SERVER_PID
wait $SERVER_PID 2>/dev/null || true
rm -f "$DB_FILE"
echo "=== All server tests passed ==="