#include "AddBookWindow.h"
#include "Connection.h"  // Incluye tu archivo Connection.h
#include <commctrl.h>
#include <string>
#include <vector>
#include <libpq-fe.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "libpq.lib")

HWND hEditFields[6];
std::wstring currentUser;

std::vector<std::wstring> fieldLabels = {
    L"Título", L"Autor", L"ISBN", L"Editorial", L"Año de Publicación", L"Materia o Área Científica"
};

// Función para convertir std::wstring a std::string
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    
    int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), 
                                 nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), 
                            &result[0], size, nullptr, nullptr);
    return result;
}

LRESULT CALLBACK AddBookWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        for (int i = 0; i < 6; ++i) {
            CreateWindowW(L"STATIC", fieldLabels[i].c_str(), WS_VISIBLE | WS_CHILD,
                20, 20 + i * 50, 180, 20, hwnd, nullptr, nullptr, nullptr);

            hEditFields[i] = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
                200, 20 + i * 50, 250, 20, hwnd, nullptr, nullptr, nullptr);
        }

        CreateWindowW(L"BUTTON", L"Agregar Libro", WS_VISIBLE | WS_CHILD,
            200, 340, 150, 30, hwnd, (HMENU)1, nullptr, nullptr);
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
            const char* paramValues[1] = { WStringToString(values[2]).c_str() };
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

            // Insertar nuevo libro
            const char* insertParams[6] = {
                WStringToString(values[0]).c_str(),
                WStringToString(values[1]).c_str(),
                WStringToString(values[2]).c_str(),
                WStringToString(values[3]).c_str(),
                WStringToString(values[4]).c_str(),
                WStringToString(values[5]).c_str()
            };

            res = PQexecParams(conn,
                "INSERT INTO libros (titulo, autor, isbn, editorial, anio, materia) VALUES ($1, $2, $3, $4, $5, $6)",
                6, nullptr, insertParams, nullptr, nullptr, 0);

            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error al insertar", MB_ICONERROR);
            } else {
                std::wstring msg = L"El libro " + values[0] + L" ha sido agregado exitosamente";
                MessageBoxW(hwnd, msg.c_str(), L"Éxito", MB_OK);
            }

            PQclear(res);
            PQfinish(conn);
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
        500, 450, 
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    
}