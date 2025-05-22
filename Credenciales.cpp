#include "Credenciales.h"
#include <fstream>
#include <iostream>

std::string Credenciales::email = "";
std::string Credenciales::appPassword = "";

bool Credenciales::cargarDesdeArchivo(const std::string& rutaArchivo) {
    std::ifstream file(rutaArchivo);
    if (!file.is_open()) {
        std::cerr << "Error: no se pudo abrir archivo " << rutaArchivo << std::endl;
        return false;
    }

    std::string linea;
    while (std::getline(file, linea)) {
        size_t pos = linea.find('=');
        if (pos == std::string::npos) continue;

        std::string key = linea.substr(0, pos);
        std::string value = linea.substr(pos + 1);

        if (key == "usuario") {
            email = value;
        } else if (key == "clave") {
            appPassword = value;
        }
    }

    file.close();

    if (email.empty() || appPassword.empty()) {
        std::cerr << "Error: usuario o clave vacíos en archivo" << std::endl;
        return false;
    }

    return true;
}

std::string Credenciales::getEmail() {
    return email;
}

std::string Credenciales::getAppPassword() {
    return appPassword;
}
