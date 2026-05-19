Проект собственной СУБД
=======================

Команда:
--------

Группа ИУ5-23Б

* Балаева Елена
* Следнев Даня
* Решетов Максим
* Шестаков Максим

Запуск проекта
--------------

Сборка:

```[shell]
./build.sh

# или

make
```
Запуск:

```[shell]
./run.sh

# или

bulid/prj.out
```

Базовые операции:

```[bash]
./dbquery mydata.csv "CREATE TABLE users (id INT, name TEXT, email TEXT);" mydata.csv
```

```[bash]
./dbquery mydata.csv "INSERT INTO users VALUES (1, 'John', 'john@example.com');" mydata.csv
./dbquery mydata.csv "INSERT INTO users VALUES (2, 'Jane', 'jane@example.com');" mydata.csv
```

```[bash]
./dbquery mydata.csv "SELECT * FROM users;"
id,name,email
1,John,john@example.com
2,Jane,jane@example.com
```
