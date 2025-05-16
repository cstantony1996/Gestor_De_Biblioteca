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

static HWND hMenuWindow = nullptr;

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

            try
            {
                string username = WStringToString(currentUser);

                const char* userParams[1] = { username.c_str() };
                PGresult* res = PQexecParams(conn,
                    "SELECT id FROM usuarios WHERE username = $1",
                    1, nullptr, userParams, nullptr, nullptr, 0);

                if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
                {
                    MessageBoxW(hwnd, L"Usuario no encontrado", L"Error", MB_ICONERROR);
                    PQclear(res); PQfinish(conn);
                    break;
                }
                int userId = atoi(PQgetvalue(res, 0, 0));
                PQclear(res);

                const char* bookParams[1] = { isbn.c_str() };
                res = PQexecParams(conn,
                    "SELECT id, titulo, estado FROM libros WHERE REGEXP_REPLACE(isbn, '[^0-9]', '', 'g') = $1",
                    1, nullptr, bookParams, nullptr, nullptr, 0);

                if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
                {
                    MessageBoxW(hwnd, L"Libro no encontrado. Verifique el ISBN.", L"Error", MB_ICONERROR);
                    PQclear(res); PQfinish(conn);
                    break;
                }

                int libroId = atoi(PQgetvalue(res, 0, 0));
                string titulo = PQgetvalue(res, 0, 1);
                string estado = PQgetvalue(res, 0, 2);
                PQclear(res);

                if (estado != "Disponible")
                {
                    MessageBoxW(hwnd, L"El libro no está disponible para préstamo.", L"Error", MB_ICONERROR);
                    PQfinish(conn);
                    break;
                }

                stringstream ssUserId, ssLibroId;
                ssUserId << userId;
                ssLibroId << libroId;

                const char* prestamoParams[3] = {
                    ssUserId.str().c_str(),
                    ssLibroId.str().c_str(),
                    fechaPostgres.c_str()
                };

                res = PQexecParams(conn,
                    "INSERT INTO prestamos (usuario_id, libro_id, fecha_devolucion) VALUES ($1::int, $2::int, $3::date)",
                    3, nullptr, prestamoParams, nullptr, nullptr, 0);

                if (PQresultStatus(res) != PGRES_COMMAND_OK)
                {
                    MessageBoxA(hwnd, PQerrorMessage(conn), "Error al registrar préstamo", MB_ICONERROR);
                    PQclear(res);
                    PQfinish(conn);
                    break;
                }
                PQclear(res);

                res = PQexecParams(conn,
                    "UPDATE libros SET estado = 'Prestado' WHERE id = $1",
                    1, nullptr, &prestamoParams[1], nullptr, nullptr, 0);

                if (PQresultStatus(res) != PGRES_COMMAND_OK)
                {
                    MessageBoxA(hwnd, PQerrorMessage(conn), "Error al actualizar libro", MB_ICONERROR);
                    PQclear(res);
                    PQfinish(conn);
                    break;
                }

                PQclear(res);
                PQfinish(conn);

                wstring mensaje = L"Préstamo registrado exitosamente\n";
                mensaje += L"Libro: " + StringToWString(titulo) + L"\n";
                mensaje += L"Fecha devolución: " + StringToWString(fechaDevolucion);
                MessageBoxW(hwnd, mensaje.c_str(), L"Éxito", MB_OK);
                SetWindowTextW(hIsbnField, L"978-");
                SetWindowTextW(hFechaDevolucionField, L"");
            }
            catch (...)
            {
                MessageBoxW(hwnd, L"Ocurrió un error inesperado", L"Error", MB_ICONERROR);
                PQfinish(conn);
            }
        }
        else if (LOWORD(wParam) == 2)
        {
            ShowWindow(hMenuWindow, SW_SHOW); // ✅ esto ahora sí funciona
    DestroyWindow(hwnd);
        }
        break;

    case WM_DESTROY:
        
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Función auxiliar que crea la ventana de préstamo
void CrearVentanaPrestamo(HINSTANCE hInstance, HWND hWndMenu)
{
    WNDCLASSW wc = { 0 };
    wc.lpszClassName = L"LoanBookWindowClass";
    wc.hInstance = hInstance;
    wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    wc.lpfnWndProc = LoanBookWndProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); // Asegúrate de tener el ícono en resources

    RegisterClassW(&wc);

    wstring titulo = L"Prestar Libro - Usuario: " + currentUser;

    HWND hwndLoan = CreateWindowW(wc.lpszClassName, titulo.c_str(),
                                  WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 575, 170,
                                  nullptr, nullptr, hInstance, nullptr);

    if (hwndLoan == nullptr)
    {
        MessageBoxW(nullptr, L"No se pudo crear la ventana de préstamo", L"Error", MB_ICONERROR);
        return;
    }

    hMenuWindow = hWndMenu;
    ShowWindow(hwndLoan, SW_SHOW);
    UpdateWindow(hwndLoan);
}

// FUNCIÓN PRINCIPAL QUE DEBES DECLARAR EN LoanBookWindow.h
void ShowLoanBookWindow(HINSTANCE hInstance, const std::wstring& username, HWND hWndMenu)
{
    currentUser = username;
    CrearVentanaPrestamo(hInstance, hWndMenu); // ✅ pasar el menú
}