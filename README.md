# SQL_Database: An SQL-like Database Emulator in C++

## Overview

**SQL_Database** is a C++ console application that emulates a simple SQL database engine using plain text files. It allows users to execute basic SQL-like commands to create tables, insert data, query data (including with filtering and simple joins), and describe table schemas—all without requiring an external database engine.

This project is ideal for learning SQL parsing, file I/O, and mimicking database behaviors using C++ and the standard library.

## Features

- **Create Table**: Define tables with column headers using a familiar SQL syntax.
- **Insert Data**: Add rows to tables using `INSERT INTO ... VALUES (...)`.
- **Select Data**: Display full tables or selected columns, with optional `WHERE` filtering.
- **Describe Table**: Show the schema (column names) of any table.
- **Join Tables**: Perform simple inner joins between two tables, including with filtering.
- **Case Insensitivity**: All SQL keywords and identifiers are case-insensitive.
- **Pure C++**: Uses only C++17 standard library features—no external dependencies.

## Getting Started

### Prerequisites
- C++17 compatible compiler (e.g., `g++`, `clang++`)
- A POSIX-compatible system (for file permission settings; on Windows, file permissions are ignored)

### Building
Compile the program using:

```sh
g++ -std=c++17 -o sql_database sql_table_manager.cpp
```

### Running
The program expects an SQL command as a single argument:

```sh
./sql_database "SQL COMMAND"
```

**Examples:**
- Create a table:
  ```sh
  ./sql_database "CREATE TABLE students (id,name,age)"
  ```

- Insert a row:
  ```sh
  ./sql_database "INSERT INTO students VALUES (1,'Alice',20)"
  ```

- Select all rows:
  ```sh
  ./sql_database "SELECT * FROM students"
  ```

- Select with filter:
  ```sh
  ./sql_database "SELECT name FROM students WHERE age>=18"
  ```

- Describe a table:
  ```sh
  ./sql_database "DESC students"
  ```

- Join two tables:
  ```sh
  ./sql_database "SELECT students.name,grades.grade FROM students JOIN grades ON students.id=grades.student_id WHERE grades.grade>='B'"
  ```

## Supported SQL Syntax
- **CREATE TABLE**:  
  `CREATE TABLE tablename (col1,col2,...)`

- **INSERT INTO**:  
  `INSERT INTO tablename VALUES (val1,'val2',...)`

- **SELECT**:  
  `SELECT col1,col2 FROM tablename`  
  `SELECT * FROM tablename WHERE col OP value`  
  (Supported operators: `=`, `!=`, `>`, `<`, `>=`, `<=`)

- **DESC**:  
  `DESC tablename`

- **JOIN**:  
  `SELECT ... FROM table1 JOIN table2 ON table1.col=table2.col [WHERE table.col OP value]`

## How It Works
- **Storage**: Each table is a `.txt` file inside a `database` directory. The first line contains column names. Each row is a CSV line of values.
- **Permissions**: On Unix systems, created files and directories are given `777` permissions for easy access.
- **Parsing**: Relies on C++ regex for parsing SQL-like commands.
- **Limitations**:  
  - No data type enforcement; all values are treated as strings.
  - No support for updating or deleting rows.
  - No transaction control, indexes, or advanced SQL features.
  - Joins are inner joins only, and all joins/filters are performed in-memory.
