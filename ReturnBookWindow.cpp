#include "byte_fix.h"
#include "ReturnBookWindow.h"
#include "Connection.h"
#include "StringUtils.h"
#include "GlobalVars.h"
#include "MenuWindow.h"
#include "resources.h"
#include "WindowUtils.h"
#include <commctrl.h>
#include <libpq-fe.h>
#include <string>
#include <stdexcept>

using namespace std;

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "libpq.lib")

static HWND hIsbnField;
static HWND hMenuWindow = nullptr;

LRESULT CALLBACK ReturnBookWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Ingrese el ISBN del libro a devolver:",
                      WS_VISIBLE | WS_CHILD, 20, 20, 250, 20, hwnd, nullptr, nullptr, nullptr);

        hIsbnField = CreateWindowW(L"EDIT", L"",
                                   WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                   280, 20, 200, 20, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Devolver Libro", WS_VISIBLE | WS_CHILD,
                      120, 60, 150, 30, hwnd, (HMENU)1, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Regresar", WS_VISIBLE | WS_CHILD,
                      280, 60, 150, 30, hwnd, (HMENU)2, nullptr, nullptr);

        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1)
        { // Botón "Devolver Libro"
            wchar_t isbnBuffer[256];
            GetWindowTextW(hIsbnField, isbnBuffer, 256);
            wstring isbnW(isbnBuffer);

            if (isbnW.empty())
            {
                MessageBoxW(hwnd, L"El ISBN no puede estar vacío", L"Error", MB_ICONERROR);
                break;
            }

            string isbnStr = WStringToString(isbnW);

            PGconn *conn = conectarDB();
            if (!conn || PQstatus(conn) != CONNECTION_OK)
            {
                MessageBoxW(hwnd, L"No se pudo conectar a la base de datos", L"Error", MB_ICONERROR);
                if (conn)
                    PQfinish(conn);
                break;
            }

            // Obtener ID del usuario
            string userStr = WStringToString(currentUser);
            PGresult *userRes = PQexecParams(conn,
                                             "SELECT id FROM usuarios WHERE email = $1",
                                             1, nullptr, (const char *[]){userStr.c_str()}, nullptr, nullptr, 0);

            if (PQresultStatus(userRes) != PGRES_TUPLES_OK || PQntuples(userRes) == 0)
            {
                PQclear(userRes);
                PQfinish(conn);
                MessageBoxW(hwnd, L"Usuario no encontrado", L"Error", MB_ICONERROR);
                break;
            }

            int userId = atoi(PQgetvalue(userRes, 0, 0));
            PQclear(userRes);

            // Buscar libro con ISBN exacto
            PGresult *libroRes = PQexecParams(conn,
                                              "SELECT id FROM libros WHERE isbn = $1",
                                              1, nullptr, (const char *[]){isbnStr.c_str()}, nullptr, nullptr, 0);

            if (PQntuples(libroRes) == 0)
            {
                PQclear(libroRes);
                PQfinish(conn);
                MessageBoxW(hwnd, L"No se encontró el libro con el ISBN ingresado", L"Error", MB_ICONERROR);
                break;
            }

            int libroId = atoi(PQgetvalue(libroRes, 0, 0));
            PQclear(libroRes);

            // Eliminar el préstamo
            PGresult *del = PQexecParams(conn,
                                         "DELETE FROM prestamos WHERE libro_id = $1 AND usuario_id = $2",
                                         2, nullptr,
                                         (const char *[]){to_string(libroId).c_str(), to_string(userId).c_str()},
                                         nullptr, nullptr, 0);

            if (PQresultStatus(del) != PGRES_COMMAND_OK)
            {
                MessageBoxW(hwnd, L"No se pudo eliminar el préstamo", L"Error", MB_ICONERROR);
                PQclear(del);
                PQfinish(conn);
                break;
            }

            PQclear(del);

            // Actualizar estado del libro (opcional)
            PGresult *upd = PQexecParams(conn,
                                         "UPDATE libros SET estado = 'Disponible' WHERE id = $1",
                                         1, nullptr, (const char *[]){to_string(libroId).c_str()}, nullptr, nullptr, 0);

            if (PQresultStatus(upd) != PGRES_COMMAND_OK)
            {
                MessageBoxW(hwnd, L"No se pudo actualizar el estado del libro", L"Advertencia", MB_ICONWARNING);
            }

            PQclear(upd);
            PQfinish(conn);

            MessageBoxW(hwnd, L"El libro ha sido devuelto con éxito", L"Éxito", MB_OK);
        }
        else if (LOWORD(wParam) == 2)
        { // Botón "Regresar"
            DestroyWindow(hwnd);
            ShowMenuWindow(GetModuleHandle(nullptr), currentUser, currentRole);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowReturnBookWindow(HINSTANCE hInstance, const wstring &username, HWND hWndMenu)
{
    currentUser = username;
    hMenuWindow = hWndMenu;

    WNDCLASSW wc = {0};
    wc.lpszClassName = L"ReturnBookWindowClass";
    wc.hInstance = hInstance;
    wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    wc.lpfnWndProc = ReturnBookWndProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

    RegisterClassW(&wc);

    wstring titulo = L"Devolver Libro - Usuario: " + username;

    HWND hwnd = CreateWindowW(
        wc.lpszClassName,
        titulo.c_str(),
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 150,
        nullptr, nullptr, hInstance, nullptr);

    WindowUtils::CenterWindow(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}