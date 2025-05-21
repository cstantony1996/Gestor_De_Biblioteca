#include "RegisterUserWindow.h"
#include "GlobalVars.h"
#include <string>
#include <libpq-fe.h>

extern PGconn* conn;
HWND hCorreo = nullptr;


void RegisterNewUser(HWND hwnd) {
    wchar_t username[100], correo[100], password[100], rol[50];
    GetWindowTextW(hUsername, username, 100);
    GetWindowTextW(hCorreo, correo, 100);
    GetWindowTextW(hPassword, password, 100);

   int sel = (int)SendMessage(hRoleCombo, CB_GETCURSEL, 0, 0);
if (sel == CB_ERR) sel = 0; // Default a Lector
const wchar_t* roles[] = { L"Lector", L"Bibliotecario" };
wcscpy_s(rol, 50, roles[sel]);


    if (wcslen(username) == 0 || wcslen(correo) == 0 || wcslen(password) == 0) {
        MessageBoxW(hwnd, L"Todos los campos son obligatorios", L"Error", MB_ICONERROR);
        return;
    }


    // Conversión de wide char (wchar_t*) a std::string (UTF-8 simple)
    std::wstring wUsername(username);
    std::wstring wCorreo(correo);
    std::wstring wPassword(password);
    std::wstring wRol(rol);

    std::string sUsername(wUsername.begin(), wUsername.end());
    std::string sCorreo(wCorreo.begin(), wCorreo.end());
    std::string sPassword(wPassword.begin(), wPassword.end());
    std::string sRol(wRol.begin(), wRol.end());

    const char* paramValues[4] = {
        sUsername.c_str(),
        sCorreo.c_str(),
        sPassword.c_str(),
        sRol.c_str()
    };

    PGresult* res = PQexecParams(conn,
        "INSERT INTO usuarios (nombre_usuario, correo, contrasena, rol) VALUES ($1, $2, $3, $4)",
        4,       // Número de parámetros
        NULL,    // Tipos (NULL si se detectan automáticamente)
        paramValues,
        NULL,    // Longitudes (NULL si son strings terminados en null)
        NULL,    // Formatos (NULL para texto)
        0        // Resultado en formato texto
    );

    if (PQresultStatus(res) == PGRES_COMMAND_OK) {
        MessageBoxW(hwnd, L"Usuario registrado exitosamente", L"Éxito", MB_OK);
        DestroyWindow(hwnd);
    } else {
        MessageBoxW(hwnd, L"Error al registrar el usuario", L"Error", MB_ICONERROR);
    }

    PQclear(res);
}

LRESULT CALLBACK RegWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateWindowW(L"STATIC", L"Usuario:", WS_VISIBLE | WS_CHILD, 20, 20, 80, 20, hwnd, NULL, NULL, NULL);
            hUsername = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 20, 200, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"Correo:", WS_VISIBLE | WS_CHILD, 20, 60, 80, 20, hwnd, NULL, NULL, NULL);
            hCorreo = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 60, 200, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"Contraseña:", WS_VISIBLE | WS_CHILD, 20, 100, 80, 20, hwnd, NULL, NULL, NULL);
            hPassword = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 110, 100, 200, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"Rol:", WS_VISIBLE | WS_CHILD, 20, 140, 80, 20, hwnd, NULL, NULL, NULL);
            hRoleCombo = CreateWindowW(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_BORDER, 110, 140, 200, 100, hwnd, NULL, NULL, NULL);
            SendMessage(hRoleCombo, CB_ADDSTRING, 0, (LPARAM)L"Lector");
            SendMessage(hRoleCombo, CB_ADDSTRING, 0, (LPARAM)L"Bibliotecario");
            SendMessage(hRoleCombo, CB_SETCURSEL, 0, 0);

            CreateWindowW(L"BUTTON", L"Registrar", WS_VISIBLE | WS_CHILD, 110, 190, 100, 30, hwnd, (HMENU)1, NULL, NULL);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                RegisterNewUser(hwnd);
            }
            break;

        case WM_DESTROY:
            DestroyWindow(hwnd);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void ShowRegisterWindow(HINSTANCE hInstance) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = RegWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"RegisterWindow";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"RegisterWindow", L"Registrar Nuevo Usuario",
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              CW_USEDEFAULT, CW_USEDEFAULT, 360, 300,
                              NULL, NULL, hInstance, NULL);
}
