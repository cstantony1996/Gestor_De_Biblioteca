#include "byte_fix.h"
#include "UserAuth.h"
#include "StringUtils.h"
#include "GlobalVars.h"
#include <stdexcept>
#include <functional>
#include <locale>  // Para conversión de wstring a string
#include <codecvt> // Para conversión de wstring a string

using namespace std;

UserAuth::UserAuth(PGconn *connection) : conn(connection), authenticated(false) {}

string UserAuth::getEmailByUsername(const string &username)
{
    string email = "";

    string query = "SELECT email FROM usuarios WHERE username = '" + username + "'";

    PGresult *res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        if (PQntuples(res) > 0)
        {
            email = PQgetvalue(res, 0, 0);
        }
    }
    PQclear(res);
    return email;
}

string UserAuth::hashPassword(const string &password)
{
    hash<string> hasher;
    size_t hashValue = hasher(password + "somesalt");
    return to_string(hashValue);
}

bool UserAuth::emailExists(const string &email)
{
    const char *query = "SELECT COUNT(*) FROM usuarios WHERE email = $1";
    const char *paramValues[1] = {email.c_str()};

    PGresult *res = PQexecParams(conn, query, 1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        PQclear(res);
        throw runtime_error("Error al verificar email: " + string(PQerrorMessage(conn)));
    }

    bool exists = stoi(PQgetvalue(res, 0, 0)) > 0;
    PQclear(res);
    return exists;
}

bool UserAuth::registerUser(const string &username, const string &password, const string &email)
{
    if (username.empty() || password.empty() || email.empty())
    {
        return false;
    }

    if (emailExists(email))
    {
        return false;
    }

    string hashedPassword = hashPassword(password);

    const char *query = "INSERT INTO usuarios (username, password, email) VALUES($1, $2, $3)";
    const char *paramValues[3] = {username.c_str(), hashedPassword.c_str(), email.c_str()};

    PGresult *res = PQexecParams(conn, query, 3, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

// Versión original para string
bool UserAuth::loginByEmail(const string &email, const string &password)
{
    if (email.empty() || password.empty())
        return false;

    const char *query = "SELECT username, password, rol FROM usuarios WHERE email = $1";
    const char *paramValues[1] = {email.c_str()};

    PGresult *res = PQexecParams(conn, query, 1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        PQclear(res);
        return false;
    }

    string storedHash = PQgetvalue(res, 0, 1);
    string role = PQgetvalue(res, 0, 2);
    string username = PQgetvalue(res, 0, 0);
    PQclear(res);

    if (hashPassword(password) != storedHash)
    {
        return false;
    }

    authenticated = true;

    // Actualizar usuario y rol en variables globales
    currentUser = email;
    currentRole = wstring(role.begin(), role.end());

    return true;
}

wstring UserAuth::getUserRole(const wstring &email)
{
    string query = "SELECT rol FROM usuarios WHERE email = '" + string(email.begin(), email.end()) + "'";
    PGresult *res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        PQclear(res);
        return L"";
    }

    string role = PQgetvalue(res, 0, 0);
    PQclear(res);
    return wstring(role.begin(), role.end());
}

// Nueva versión para wstring (Windows)
bool UserAuth::loginByEmail(const wstring &email, const wstring &password)
{
    return loginByEmail(WStringToString(email), WStringToString(password));
}

bool UserAuth::isAuthenticated() const
{
    return authenticated;
}

string UserAuth::getCurrentUserEmail() const
{
    return currentUser;
}