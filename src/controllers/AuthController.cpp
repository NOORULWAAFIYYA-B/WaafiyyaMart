#include "controllers/AuthController.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

AuthController::AuthController(Database& db)
    : userRepository_(db) {}

std::string AuthController::hashPassword(const std::string& password) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);

    std::ostringstream oss;
    for (unsigned char byte : hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

bool AuthController::registerUser(const std::string& fullName, const std::string& email, const std::string& password) {
    if (fullName.empty() || email.empty() || password.empty()) {
        return false;
    }

    return userRepository_.createUser(fullName, email, hashPassword(password));
}

bool AuthController::loginUser(const std::string& email, const std::string& password) {
    if (email.empty() || password.empty()) {
        return false;
    }

    std::string storedHash = userRepository_.getPasswordHashByEmail(email);
    if (storedHash.empty()) {
        return false;
    }

    return storedHash == hashPassword(password);
}
