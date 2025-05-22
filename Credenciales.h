#ifndef CREDENCIALES_H
#define CREDENCIALES_H

#include <string>

using namespace std;

class Credenciales
{
public:
    static bool cargarDesdeArchivo(const string &rutaArchivo);
    static string getEmail();
    static string getAppPassword();

private:
    static string email;
    static string appPassword;
};

#endif
