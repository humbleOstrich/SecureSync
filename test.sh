#!/bin/bash

set -e

TESTDIR="/tmp/db_test"
DBQUERY="./build/dbquery -q"

rm -rf "$TESTDIR"
mkdir -p "$TESTDIR"

run_test() {
    local name="$1"
    shift
    echo "Тестирование $name:"
    local step=1
    while [ $# -gt 0 ]; do
        local cmd="$1"
        shift
        if eval "$cmd" > /dev/null 2>&1; then
            echo "  $step. PASS"
        else
            echo "  $step. FAIL: $cmd"
            exit 1
        fi
        ((step++))
    done
    echo ""
}

run_test "базовых операций" \
    "$DBQUERY $TESTDIR/test.csv 'CREATE TABLE users (id INT, name TEXT);' $TESTDIR/t1.csv" \
    "$DBQUERY $TESTDIR/t1.csv 'SELECT * FROM users;' | diff -q - <(printf 'id,name\n')" \
    "$DBQUERY $TESTDIR/t1.csv 'INSERT INTO users VALUES (1, 'Alice');' $TESTDIR/t2.csv" \
    "$DBQUERY $TESTDIR/t2.csv 'SELECT * FROM users;' | diff -q - <(printf 'id,name\n1,Alice\n')" \
    "$DBQUERY $TESTDIR/t2.csv 'DELETE FROM users WHERE id = 1;' $TESTDIR/t3.csv" \
    "$DBQUERY $TESTDIR/t3.csv 'SELECT * FROM users;' | diff -q - <(printf 'id,name\n')" \
    "$DBQUERY $TESTDIR/t3.csv 'DROP TABLE users;' $TESTDIR/t4.csv" \
    "! $DBQUERY $TESTDIR/t4.csv 'SELECT * FROM users;' 2>/dev/null"

run_test "множественного INSERT" \
    "$DBQUERY $TESTDIR/test.csv 'CREATE TABLE items (id INT, val TEXT);' $TESTDIR/im1.csv" \
    "$DBQUERY $TESTDIR/im1.csv 'INSERT INTO items VALUES (1, 'one'), (2, 'two'), (3, 'three');' $TESTDIR/im2.csv" \
    "$DBQUERY $TESTDIR/im2.csv 'SELECT * FROM items;' | diff -q - <(printf 'id,val\n1,one\n2,two\n3,three\n')"

run_test "UPDATE" \
    "$DBQUERY $TESTDIR/test.csv 'CREATE TABLE scores (id INT, points INT);' $TESTDIR/up1.csv" \
    "$DBQUERY $TESTDIR/up1.csv 'INSERT INTO scores VALUES (1, 10), (2, 20), (3, 30);' $TESTDIR/up2.csv" \
    "$DBQUERY $TESTDIR/up2.csv 'UPDATE scores SET points = 100;' $TESTDIR/up3.csv" \
    "$DBQUERY $TESTDIR/up3.csv 'SELECT * FROM scores;' | diff -q - <(printf 'id,points\n1,100\n2,100\n3,100\n')" \
    "$DBQUERY $TESTDIR/up3.csv 'UPDATE scores SET points = 999 WHERE id = 2;' $TESTDIR/up4.csv" \
    "$DBQUERY $TESTDIR/up4.csv 'SELECT * FROM scores;' | diff -q - <(printf 'id,points\n1,100\n2,999\n3,100\n')"

run_test "сложных условий WHERE" \
    "$DBQUERY $TESTDIR/test.csv 'CREATE TABLE products (id INT, price INT, name TEXT);' $TESTDIR/cond1.csv" \
    "$DBQUERY $TESTDIR/cond1.csv 'INSERT INTO products VALUES (1, 100, 'A'), (2, 200, 'B'), (3, 300, 'C'), (4, 150, 'D');' $TESTDIR/cond2.csv" \
    "$DBQUERY $TESTDIR/cond2.csv 'SELECT * FROM products WHERE id > 1 AND price < 250;' | diff -q - <(printf 'id,price,name\n2,200,B\n4,150,D\n')" \
    "$DBQUERY $TESTDIR/cond2.csv 'SELECT * FROM products WHERE price = 100 OR price = 300;' | diff -q - <(printf 'id,price,name\n1,100,A\n3,300,C\n')" \
    "$DBQUERY $TESTDIR/cond2.csv 'SELECT * FROM products WHERE NOT (price = 200);' | diff -q - <(printf 'id,price,name\n1,100,A\n3,300,C\n4,150,D\n')" \
    "$DBQUERY $TESTDIR/cond2.csv 'SELECT * FROM products WHERE price != 200;' | diff -q - <(printf 'id,price,name\n1,100,A\n3,300,C\n4,150,D\n')" \
    "$DBQUERY $TESTDIR/cond2.csv 'SELECT * FROM products WHERE price >= 200;' | diff -q - <(printf 'id,price,name\n2,200,B\n3,300,C\n')" \
    "$DBQUERY $TESTDIR/cond2.csv 'SELECT * FROM products WHERE price <= 150;' | diff -q - <(printf 'id,price,name\n1,100,A\n4,150,D\n')" \
    "$DBQUERY $TESTDIR/cond2.csv 'SELECT * FROM products WHERE (id = 1 OR id = 2) AND price < 300;' | diff -q - <(printf 'id,price,name\n1,100,A\n2,200,B\n')"

run_test "CREATE/DROP DATABASE" \
    "$DBQUERY $TESTDIR/dummy.csv 'CREATE DATABASE mydb;'" \
    "test -f mydb.db" \
    "$DBQUERY $TESTDIR/dummy.csv 'DROP DATABASE mydb;'" \
    "test ! -f mydb.db"

run_test "типов данных" \
    "$DBQUERY $TESTDIR/test.csv 'CREATE TABLE types (i INT, f FLOAT, b BOOL, t TEXT, v VARCHAR(10));' $TESTDIR/types1.csv" \
    "$DBQUERY $TESTDIR/types1.csv \"INSERT INTO types (i, f, b, t, v) VALUES (123, 3.14, true, 'hello', 'world');\" $TESTDIR/types2.csv" \
    "$DBQUERY $TESTDIR/types2.csv 'SELECT * FROM types;' | diff -q - <(printf 'i,f,b,t,v\n123,3.14,true,hello,world\n')" \
    "! $DBQUERY $TESTDIR/types2.csv 'INSERT INTO types VALUES (abc, 1.0, true, 'x', 'y');' $TESTDIR/types3.csv 2>/dev/null" \
    "! $DBQUERY $TESTDIR/types2.csv 'UPDATE types SET i = 12.34;' $TESTDIR/types4.csv 2>/dev/null"

run_test "явного списка столбцов INSERT" \
    "$DBQUERY $TESTDIR/test.csv 'CREATE TABLE cols (a INT, b TEXT, c INT);' $TESTDIR/cols1.csv" \
    "$DBQUERY $TESTDIR/cols1.csv 'INSERT INTO cols (b, a) VALUES ('text', 42);' $TESTDIR/cols2.csv" \
    "$DBQUERY $TESTDIR/cols2.csv 'SELECT * FROM cols;' | diff -q - <(printf 'a,b,c\n42,text,\n')"

echo "=== All console tests passed ==="