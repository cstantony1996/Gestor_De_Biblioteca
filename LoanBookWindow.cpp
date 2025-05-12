#include "LoanBookWindow.h"
#include "Connection.h"
#include "MenuWindow.h"
#include "StringUtils.h"
#include "GlobalVars.h"
#include "resources.h"
#include <commctrl.h>
#include <string>
#include <libpq-fe.h>
#include <ctime>
#include <sstream>

using namespace std;

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "libpq.lib")

HWND hIsbnField;
HWND hFechaDevolucionField;

// Convierte fecha DD-MM-YYYY a YYYY-MM-DD
string ConvertirFechaFormatoPostgres(const string& fechaStr)
{
    int dia, mes, año;
    if (sscanf(fechaStr.c_str(), "%d-%d-%d", &dia, &mes, &año) != 3)
        return "";

    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", año, mes, dia);
    return string(buffer);
}

bool ValidarFechaDevolucion(HWND hwnd, const string& fechaStr)
{
    int dia, mes, año;
    if (sscanf(fechaStr.c_str(), "%d-%d-%d", &dia, &mes, &año) != 3)
    {
        MessageBoxW(hwnd, L"Formato de fecha inválido. Use DD-MM-YYYY", L"Error", MB_ICONERROR);
        return false;
    }

    tm fechaDevolucion = {};
    fechaDevolucion.tm_year = año - 1900;
    fechaDevolucion.tm_mon = mes - 1;
    fechaDevolucion.tm_mday = dia;

    time_t tDevolucion = mktime(&fechaDevolucion);
    time_t tActual = time(nullptr);

    if (tDevolucion < tActual)
    {
        MessageBoxW(hwnd, L"La fecha de devolución no puede ser anterior a hoy", L"Error", MB_ICONERROR);
        return false;
    }

    return true;
}

string ProcesarISBN(HWND hwnd, const wstring& isbnW)
{
    wstring cleanIsbn;
    for (wchar_t c : isbnW)
        if (iswdigit(c))
            cleanIsbn += c;

    if (cleanIsbn.empty())
    {
        MessageBoxW(hwnd, L"El ISBN no puede estar vacío", L"Error", MB_ICONERROR);
        return "";
    }

    if (cleanIsbn.length() < 10)
    {
        MessageBoxW(hwnd, L"ISBN demasiado corto. Debe tener al menos 10 dígitos.", L"Error", MB_ICONERROR);
        return "";
    }

    return WStringToString(cleanIsbn);
}

LRESULT CALLBACK LoanBookWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Ingrese el ISBN del libro a prestar:",
                      WS_VISIBLE | WS_CHILD, 20, 20, 280, 20, hwnd, nullptr, nullptr, nullptr);

        hIsbnField = CreateWindowW(L"EDIT", L"978-",
                                   WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                   300, 20, 200, 20, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"STATIC", L"Fecha de devolución (DD-MM-YYYY):",
                      WS_VISIBLE | WS_CHILD, 20, 50, 280, 20, hwnd, nullptr, nullptr, nullptr);

        hFechaDevolucionField = CreateWindowW(L"EDIT", L"",
                                              WS_VISIBLE | WS_CHILD | WS_BORDER,
                                              300, 50, 200, 20, hwnd, nullptr, nullptr, nullptr);

        const int BUTTON_WIDTH = 150;
        const int BUTTON_HEIGHT = 30;
        const int BUTTON_Y = 90;
        const int TOTAL_BUTTONS_WIDTH = 2 * BUTTON_WIDTH + 20;
        const int START_X = (575 - TOTAL_BUTTONS_WIDTH) / 2;

        CreateWindowW(L"BUTTON", L"Regresar", WS_VISIBLE | WS_CHILD,
                      START_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT,
                      hwnd, (HMENU)2, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Prestar Libro", WS_VISIBLE | WS_CHILD,
                      START_X + BUTTON_WIDTH + 20, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT,
                      hwnd, (HMENU)1, nullptr, nullptr);
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1)
        {
            wchar_t isbnBuffer[256];
            GetWindowTextW(hIsbnField, isbnBuffer, 256);
            wstring isbnW(isbnBuffer);

            string isbn = ProcesarISBN(hwnd, isbnW);
            if (isbn.empty()) break;

            wchar_t fechaBuffer[256];
            GetWindowTextW(hFechaDevolucionField, fechaBuffer, 256);
            string fechaDevolucion = WStringToString(fechaBuffer);

            if (!ValidarFechaDevolucion(hwnd, fechaDevolucion)) break;

            string fechaPostgres = ConvertirFechaFormatoPostgres(fechaDevolucion);
            if (fechaPostgres.empty())
            {
                MessageBoxW(hwnd, L"Error al convertir la fecha", L"Error", MB_ICONERROR);
                break;
            }

            PGconn* conn = conectarDB();
            if (PQstatus(conn) != CONNECTION_OK)
            {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error de conexión", MB_ICONERROR);
                PQfinish(conn);
                break;
            }

            const char* paramValues[1] = { isbn.c_str() };
            PGresult* res = PQexecParams(conn,
                                         "SELECT titulo, estado FROM libros WHERE REGEXP_REPLACE(isbn, '[^0-9]', '', 'g') = $1",
                                         1, nullptr, paramValues, nullptr, nullptr, 0);

            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                MessageBoxW(hwnd, L"Libro no encontrado. Verifique el ISBN.", L"Error", MB_ICONERROR);
                PQclear(res);
                PQfinish(conn);
                break;
            }

            string tituloLibro = PQgetvalue(res, 0, 0);
            string estado = PQgetvalue(res, 0, 1);
            PQclear(res);

            if (estado != "Disponible")
            {
                MessageBoxW(hwnd, L"El libro no está disponible para préstamo.", L"Error", MB_ICONERROR);
                PQfinish(conn);
                break;
            }

            string username = WStringToString(currentUser);
            const char* insertParams[5] = {
                isbn.c_str(),
                tituloLibro.c_str(),
                username.c_str(),
                "now()",
                fechaPostgres.c_str()
            };

            res = PQexecParams(conn,
                               "INSERT INTO prestamos (isbn, titulo, usuario, fecha_prestamo, fecha_devolucion) "
                               "VALUES ($1, $2, $3, $4::timestamp, $5::date)",
                               5, nullptr, insertParams, nullptr, nullptr, 0);

            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error al registrar préstamo", MB_ICONERROR);
                PQclear(res);
                PQfinish(conn);
                break;
            }
            PQclear(res);

            res = PQexecParams(conn,
                               "UPDATE libros SET estado = 'Prestado' WHERE REGEXP_REPLACE(isbn, '[^0-9]', '', 'g') = $1",
                               1, nullptr, paramValues, nullptr, nullptr, 0);

            if (PQresultStatus(res) == PGRES_COMMAND_OK)
            {
                wstring mensaje = L"Préstamo registrado exitosamente\n";
                mensaje += L"Libro: " + StringToWString(tituloLibro) + L"\n";
                mensaje += L"Fecha devolución: " + StringToWString(fechaDevolucion);
                MessageBoxW(hwnd, mensaje.c_str(), L"Éxito", MB_OK);
                SetWindowTextW(hIsbnField, L"978-");
                SetWindowTextW(hFechaDevolucionField, L"");
            }
            else
            {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error al actualizar estado", MB_ICONERROR);
            }

            PQclear(res);
            PQfinish(conn);
        }
        else if (LOWORD(wParam) == 2)
        {
            DestroyWindow(hwnd);
            ShowMenuWindow(GetModuleHandle(nullptr), currentUser);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowLoanBookWindow(HINSTANCE hInstance, const wstring& username)
{
    currentUser = username;

    const wchar_t CLASS_NAME[] = L"LoanBookWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = LoanBookWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    RegisterClassW(&wc);

    wstring title = L"Préstamo de Libros - Usuario: " + username;

    HWND hwnd = CreateWindowW(
        CLASS_NAME,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        575, 180,
        nullptr, nullptr, hInstance, nullptr);

    SendMessage(hwnd, WM_SETICON, ICON_BIG, 
    (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL,
        (LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), 
        IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}
