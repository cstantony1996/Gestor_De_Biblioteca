#include "byte_fix.h"
#include "Credenciales.h"
#include <fstream>
#include <iostream>

using namespace std;

string Credenciales::email = "";
string Credenciales::appPassword = "";

bool Credenciales::cargarDesdeArchivo(const string &rutaArchivo)
{
    ifstream file(rutaArchivo);
    if (!file.is_open())
    {
        cerr << "Error: no se pudo abrir archivo " << rutaArchivo << endl;
        return false;
    }

    string linea;
    while (getline(file, linea))
    {
        size_t pos = linea.find('=');
        if (pos == string::npos)
            continue;

        string key = linea.substr(0, pos);
        string value = linea.substr(pos + 1);

        if (key == "usuario")
        {
            email = value;
        }
        else if (key == "clave")
        {
            appPassword = value;
        }
    }

    file.close();

    if (email.empty() || appPassword.empty())
    {
        cerr << "Error: usuario o clave vacíos en archivo" << endl;
        return false;
    }

    return true;
}

string Credenciales::getEmail()
{
    return email;
}

string Credenciales::getAppPassword()
{
    return appPassword;
}
