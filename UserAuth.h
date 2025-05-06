#ifndef USERAUTH_H
#define USERAUTH_H

#include <libpq-fe.h>
#include <string>
#include <ctime>

using namespace std;

class UserAuth
{
private:
    PGconn *conn;

    string hashPassword(const string &password);
    bool usernameExists(const string &username);
    bool emailExists(const string &email);

public:
    UserAuth(PGconn *connection);
    bool registerUser(const string &username, const string &password, const string &email);
    bool login(const string &username, const string &password);
    void showAuthMenu();
    bool isAuthenticated() const;

private:
    bool authenticated = false;
    string currentUser;
};

#endif // USERAUTH_H