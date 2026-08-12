#include <iostream>
#include <string>
#include <vector>

#include "controllers/AuthController.h"
#include "database/Database.h"

void printUsage() {
    std::cout << "Usage:\n";
    std::cout << "  --register <full_name> <email> <password>\n";
    std::cout << "  --login <email> <password>\n";
    std::cout << "  --seed\n";
}

int main(int argc, char* argv[]) {
    const std::string dbPath = "waafiyyamart.db";
    Database db(dbPath);
    if (!db.open()) {
        std::cerr << "Database could not be opened." << std::endl;
        return 1;
    }

    const std::string schema =
        "PRAGMA foreign_keys = ON;"
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "full_name TEXT NOT NULL,"
        "email TEXT NOT NULL UNIQUE,"
        "password_hash TEXT NOT NULL,"
        "role TEXT NOT NULL DEFAULT 'customer',"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);";

    if (!db.execute(schema)) {
        std::cerr << "Schema creation failed." << std::endl;
        return 1;
    }

    AuthController auth(db);

    if (argc < 2) {
        printUsage();
        return 0;
    }

    std::string command = argv[1];

    if (command == "--register" && argc == 5) {
        const std::string fullName = argv[2];
        const std::string email = argv[3];
        const std::string password = argv[4];

        if (auth.registerUser(fullName, email, password)) {
            std::cout << "User registered successfully." << std::endl;
            return 0;
        }

        std::cout << "Registration failed. User may already exist." << std::endl;
        return 1;
    }

    if (command == "--login" && argc == 4) {
        const std::string email = argv[2];
        const std::string password = argv[3];

        if (auth.loginUser(email, password)) {
            std::cout << "Login successful." << std::endl;
            return 0;
        }

        std::cout << "Invalid email or password." << std::endl;
        return 1;
    }

    if (command == "--seed") {
        auth.registerUser("Admin User", "admin@waafiyyamart.com", "admin123");
        auth.registerUser("Customer One", "customer1@example.com", "customer123");
        std::cout << "Seed users created." << std::endl;
        return 0;
    }

    printUsage();
    return 1;
}
