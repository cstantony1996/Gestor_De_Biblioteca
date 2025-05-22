#ifndef USERAUTH_H
#define USERAUTH_H

#include "byte_fix.h" 
#include <string>
#include <libpq-fe.h>

using namespace std;

class UserAuth
{
private:
    PGconn *conn;
    bool authenticated;
    string currentUser;

public:
    UserAuth(PGconn *connection);
    bool loginByEmail(const string &email, const string &password);
    bool loginByEmail(const wstring &email, const wstring &password); // Nueva sobrecarga
    bool registerUser(const string &username, const string &password, const string &email);
    bool isAuthenticated() const;
    string getCurrentUserEmail() const;
    wstring getUserRole(const wstring& email);
    string getEmailByUsername(const std::string& username);

private:
    string hashPassword(const string &password);
    bool emailExists(const string &email);
};

#endif