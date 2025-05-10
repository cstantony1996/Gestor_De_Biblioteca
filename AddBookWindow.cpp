#include "AddBookWindow.h"
#include "Connection.h"  // Incluye tu archivo Connection.h
#include "MenuWindow.h"
#include "StringUtils.h"
#include "GlobalVars.h"
#include <commctrl.h>
#include <string>
#include <vector>
#include <libpq-fe.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "libpq.lib")

HWND hEditFields[6];

std::vector<std::wstring> fieldLabels = {
    L"Título", L"Autor", L"ISBN", L"Editorial", L"Año de Publicación", L"Materia o Área Científica"
};

LRESULT CALLBACK AddBookWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        for (int i = 0; i < 6; ++i) {
            CreateWindowW(L"STATIC", fieldLabels[i].c_str(), WS_VISIBLE | WS_CHILD,
                20, 20 + i * 50, 180, 20, hwnd, nullptr, nullptr, nullptr);

                const wchar_t* defaultText = (i == 2 ? L"978- -  -     - " : L"");

                int width = (i == 0) ? 295 : 250;
                hEditFields[i] = CreateWindowW(L"EDIT", defaultText, WS_VISIBLE | WS_CHILD | WS_BORDER,
                200, 20 + i * 50, width, 20, hwnd, nullptr, nullptr, nullptr);
                
        }

        // Calcula posiciones centradas
const int BUTTON_WIDTH = 150;
const int BUTTON_HEIGHT = 30;
const int BUTTON_Y = 340;  // Misma posición vertical
const int TOTAL_BUTTONS_WIDTH = 2 * BUTTON_WIDTH + 20;  // 20px de separación
const int START_X = (575 - TOTAL_BUTTONS_WIDTH) / 2;  // Centrado horizontal

// Botón "Regresar" (izquierda)
CreateWindowW(L"BUTTON", L"Regresar", WS_VISIBLE | WS_CHILD,
    START_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, 
    hwnd, (HMENU)2, nullptr, nullptr);

// Botón "Agregar Libro" (derecha)
CreateWindowW(L"BUTTON", L"Agregar Libro", WS_VISIBLE | WS_CHILD,
    START_X + BUTTON_WIDTH + 20, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT,
    hwnd, (HMENU)1, nullptr, nullptr);
            
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            std::wstring values[6];
            bool empty = false;
            for (int i = 0; i < 6; ++i) {
                wchar_t buffer[256];
                GetWindowTextW(hEditFields[i], buffer, 256);
                values[i] = buffer;
                if (values[i].empty()) {
                    MessageBoxW(hwnd, (fieldLabels[i] + L" es requerido, favor completar la información").c_str(), L"Error", MB_ICONERROR);
                    empty = true;
                    break;
                }
            }

            if (empty) break;

            PGconn* conn = conectarDB();
            
            if (PQstatus(conn) != CONNECTION_OK) {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error de conexión", MB_ICONERROR);
                PQfinish(conn);
                break;
            }

            // Verificar si el ISBN ya existe
            std::string isbnStr = WStringToString(values[2]);
            const char* paramValues[1] = { isbnStr.c_str() };

            PGresult* res = PQexecParams(conn,
                "SELECT COUNT(*) FROM libros WHERE isbn = $1",
                1, nullptr, paramValues, nullptr, nullptr, 0);

            if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error en consulta", MB_ICONERROR);
                PQclear(res);
                PQfinish(conn);
                break;
            }

            if (atoi(PQgetvalue(res, 0, 0)) > 0) {
                MessageBoxW(hwnd, L"ISBN ya está registrado, favor verificar", L"Error", MB_ICONERROR);
                PQclear(res);
                PQfinish(conn);
                break;
            }
            PQclear(res);

            std::string titulo     = WStringToString(values[0]);
            std::string autor      = WStringToString(values[1]);
            std::string isbn       = WStringToString(values[2]);
            std::string editorial  = WStringToString(values[3]);
            std::string año        = WStringToString(values[4]);
            std::string materia    = WStringToString(values[5]);
            
            const char* insertParams[6] = {
                titulo.c_str(),
                autor.c_str(),
                isbn.c_str(),
                editorial.c_str(),
                año.c_str(),
                materia.c_str()
            };

            OutputDebugStringA(("Título: " + titulo + "\n").c_str());
OutputDebugStringA(("Autor: " + autor + "\n").c_str());
OutputDebugStringA(("ISBN: " + isbn + "\n").c_str());
OutputDebugStringA(("Editorial: " + editorial + "\n").c_str());
OutputDebugStringA(("Año: " + año + "\n").c_str());
OutputDebugStringA(("Materia: " + materia + "\n").c_str());


res = PQexecParams(conn,
    "INSERT INTO libros (titulo, autor, isbn, editorial, año, materia) VALUES ($1, $2, $3, $4, $5, $6)",
    6, nullptr, insertParams, nullptr, nullptr, 0);

if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    MessageBoxA(hwnd, PQerrorMessage(conn), "Error al insertar", MB_ICONERROR);
} else {
    std::wstring msg = L"El libro " + values[0] + L" ha sido agregado exitosamente";
    MessageBoxW(hwnd, msg.c_str(), L"Éxito", MB_OK);

    // Limpiar campos
    for (int i = 0; i < 6; ++i) {
        SetWindowTextW(hEditFields[i], (i == 2 ? L"978- -  -     - " : L""));
    }
}

PQclear(res);
PQfinish(conn);
} else if (LOWORD(wParam) == 2) { // Botón Regresar
DestroyWindow(hwnd); // Cierra esta ventana
ShowMenuWindow(GetModuleHandle(nullptr), currentUser); // Vuelve al menú
}
break;

case WM_DESTROY:
PostQuitMessage(0);
break;
}
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowAddBookWindow(HINSTANCE hInstance, const std::wstring& username) {
    currentUser = username;
    const wchar_t CLASS_NAME[] = L"AddBookWindow";
    
    WNDCLASSW wc = { };
    wc.lpfnWndProc = AddBookWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    std::wstring title = L"Agregar Libro - Usuario: " + username;

    HWND hwnd = CreateWindowW(
        CLASS_NAME, 
        title.c_str(), 
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        575, 450, 
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    
}