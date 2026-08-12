# WaafiyyaMart Backend

This project contains a simple C++ backend for WaafiyyaMart using SQLite.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

Register a user:

```bash
./build/waafiyyamart --register "Jane Doe" jane@example.com pass123
```

Login a user:

```bash
./build/waafiyyamart --login jane@example.com pass123
```

Initialize the database with default sample data:

```bash
./build/waafiyyamart --seed
```
