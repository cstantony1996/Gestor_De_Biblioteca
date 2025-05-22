#ifndef CREDENCIALES_H
#define CREDENCIALES_H

#include <string>

class Credenciales {
public:
    static bool cargarDesdeArchivo(const std::string& rutaArchivo);
    static std::string getEmail();
    static std::string getAppPassword();
private:
    static std::string email;
    static std::string appPassword;
};

#endif
