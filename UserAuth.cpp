#include "UserAuth.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <limits>
#include <functional>

using namespace std;

UserAuth::UserAuth(PGconn* connection) : conn(connection) {}

string UserAuth::hashPassword(const string& password) {
    
    hash<string> hasher;
    size_t hashValue = hasher(password + "somesalt");
    return to_string(hashValue);
}

bool UserAuth::usernameExists(const string& username) {
    const char* query = "SELECT COUNT(*) FROM usuarios WHERE username = $1";
    const char* paramValues[1] = {username.c_str()};

    PGresult* res = PQexecParams(conn, query, 1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        throw runtime_error("Error al verificar username: " + string(PQerrorMessage(conn)));
    }

    bool exists = stoi(PQgetvalue(res, 0, 0)) > 0;
    PQclear(res);
    return exists;
}

bool UserAuth::emailExists(const string& email) {
    const char* query = "SELECT COUNT(*) FROM usuarios WHERE email = $1";
    const char* paramValues[1] = {email.c_str()};

    PGresult* res = PQexecParams(conn, query, 1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        throw runtime_error("Error al verificar email: " + string(PQerrorMessage(conn)));
    }

    bool exists = stoi(PQgetvalue(res, 0, 0)) > 0;
    PQclear(res);
    return exists;
}

bool UserAuth::registerUser(const string& username, const string& password, const string& email) {
    if (username.empty() || password.empty() || email.empty()) {
        cout << "⚠️ Error: Debes llenar todos los campos.\n" << endl;
        return false;
    }

    if (usernameExists(username)) {
        cout << "⚠️ Error: El nombre de usuario ya existe.\n" << endl;
        return false;
    }

    if (emailExists(email)) {
        cout << "⚠️ Error: El correo ya está registrado.\n" << endl;
        return false;
    }

    string hashedPassword = hashPassword(password);

    const char* query = "INSERT INTO usuarios (username, password, email) VALUES($1, $2, $3)";
    const char* paramValues[3] = {username.c_str(), hashedPassword.c_str(), email.c_str()};

    PGresult* res = PQexecParams(conn, query, 3, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cout << "⚠️ Error al registrar: " << PQerrorMessage(conn) << "\n";
        PQclear(res);
        return false;
    }

    PQclear(res);
    cout << "✅ Registro exitoso! Ahora puedes iniciar sesión.\n" << endl;
    return true;
}

bool UserAuth::login(const string& username, const string& password) {
    if (username.empty() || password.empty()) {
        cout << "⚠️ Error: Usuario y contraseña requeridos.\n" << endl;
        return false;
    }

    const char* query = "SELECT id, password FROM usuarios WHERE username = $1";
    const char* paramValues[1] = {username.c_str()};

    PGresult* res = PQexecParams(conn, query, 1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        cout << "⚠️ Error de auntenticación.\n";
        return false;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        cout << "⚠️ Usuario no encontrado.\n";
        return false;
    }

    string storedHash = PQgetvalue(res, 0, 1);
    PQclear(res);

    if (hashPassword(password) != storedHash) {
        cout << "⚠️ Contraseña incorrecta.\n";
        return false;
    }

    authenticated = true;
    currentUser = username;
    cout << "✅ Bienvenido, " << username << "!\n" << endl;
    return true;
}

bool UserAuth::isAuthenticated() const {
    return authenticated;
}

void UserAuth::showAuthMenu() {
    int opcion;
    string username, password, email;

    while(true) {
        cout << "\n════════ MENÚ DE ACCESO ════════\n";
        cout << "1. Iniciar sesión.\n";
        cout << "2. Registrarse.\n";
        cout << "3. Salir.\n";
        cout << "────────────────────────────────\n";
        cout << "Seleccione una opción: ";

        if (!(cin >> opcion)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "⚠️ Error: Debes ingresar un número.\n";
            continue;
        }
        cin.ignore();

        switch(opcion) {
            case 1:
                cout << "Usuario: ";
                getline(cin, username);
                cout << "Contraseña: ";
                getline(cin, password);

                if (login(username, password)) {
                    return;    
                }
                break;

            case 2:
                cout << "Nuevo usuario.\n";
                cout << "Usuario: ";
                getline(cin, username);
                cout << "Email: ";
                getline(cin, email);
                cout << "Contraseña: ";
                getline(cin, password);

                registerUser(username, password, email);
                break;

            case 3:
                cout << "Saliendo del sistema...\n";
                exit(0);

            default:
                cout << "⚠️ Error: Opción inválida.\n";
            
        }
    }
}