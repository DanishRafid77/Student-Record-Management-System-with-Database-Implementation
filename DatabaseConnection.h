#ifndef DATABASECONNECTION_H
#define DATABASECONNECTION_H
#include <mysql.h>
extern MYSQL* conn; // Declaration of the global variable

class db_response {
public:
    static void ConnectionFunction();
    static void Cleanup();
};

#endif // DATABASECONNECTION_H