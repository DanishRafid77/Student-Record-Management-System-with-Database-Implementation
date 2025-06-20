#include <iostream>
#include "DatabaseConnection.h"

MYSQL* conn = nullptr; // Define the global variable

void db_response::ConnectionFunction() {
    if (mysql_library_init(0, NULL, NULL)) {
        std::cerr << "Could not initialize MySQL library" << std::endl;
        return;
    }

    conn = mysql_init(nullptr);
    if (!conn) {
        std::cerr << "MySQL Initialization Failed" << std::endl;
        mysql_library_end();
        return;
    }

    // Attempt to connect to the database
    if (!mysql_real_connect(conn, "localhost", "root", "", "student_record_management_system", 3306, nullptr, 0)) {
        std::cerr << "Failed to connect to MySQL! Error: " << mysql_error(conn) << std::endl;
        mysql_close(conn); // Free resources even if connection fails
        conn = nullptr;
        mysql_library_end();
        return;
    }

    std::cout << "Database connected successfully!" << std::endl;
}

void db_response::Cleanup() {
    if (conn) {
        mysql_close(conn); // Close connection if open
        conn = nullptr;    // Avoid dangling pointers
    }
    mysql_library_end(); // Clean up library
}
