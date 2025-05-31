#include "byte_fix.h"
#include "LoanBookWindow.h"
#include "Connection.h"
#include "MenuWindow.h"
#include "StringUtils.h"
#include "GlobalVars.h"
#include "resources.h"
#include "WindowUtils.h"
#include "EmailSender.h"
#include <commctrl.h>
#include <string>
#include <libpq-fe.h>
#include <iostream>
#include <ctime>
#include <sstream>
#include <algorithm>

using namespace std;

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "libpq.lib")

static HWND hMenuWindow = nullptr;
static HWND hIsbnField;
static HWND hFechaDevolucionField;
static HWND hUsernameLectorField;
static HWND hTituloField;
static HWND hBuscarButton;
static HWND hResultadoListView;

string ConvertirFechaFormatoPostgres(const string &fechaStr)
{
    int dia, mes, anio;
    if (sscanf(fechaStr.c_str(), "%d-%d-%d", &dia, &mes, &anio) != 3)
        return "";

    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", anio, mes, dia);
    return string(buffer);
}

bool ValidarFechaDevolucion(HWND hwnd, const string &fechaStr)
{
    int dia, mes, anio;
    if (sscanf(fechaStr.c_str(), "%d-%d-%d", &dia, &mes, &anio) != 3)
    {
        MessageBoxW(hwnd, L"Formato de fecha inválido. Use DD-MM-YYYY", L"Error", MB_ICONERROR);
        return false;
    }

    tm fechaDevolucion = {};
    fechaDevolucion.tm_year = anio - 1900;
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

string ProcesarISBN(HWND hwnd, const wstring &isbnW)
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
        InitCommonControls();

        CreateWindowW(L"STATIC", L"Ingrese el ISBN del libro a prestar:", WS_VISIBLE | WS_CHILD, 20, 20, 280, 20, hwnd, nullptr, nullptr, nullptr);
        hIsbnField = CreateWindowW(L"EDIT", L"978-", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 300, 20, 200, 20, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"STATIC", L"Fecha de devolución (DD-MM-YYYY):", WS_VISIBLE | WS_CHILD, 20, 50, 280, 20, hwnd, nullptr, nullptr, nullptr);
        hFechaDevolucionField = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 300, 50, 200, 20, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"STATIC", L"Username del lector:", WS_VISIBLE | WS_CHILD, 20, 80, 280, 20, hwnd, nullptr, nullptr, nullptr);
        hUsernameLectorField = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 300, 80, 200, 20, hwnd, nullptr, nullptr, nullptr);

        // Nuevos controles para búsqueda
        CreateWindowW(L"STATIC", L"Título del libro:", WS_VISIBLE | WS_CHILD, 20, 110, 120, 20, hwnd, nullptr, nullptr, nullptr);
        hTituloField = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 150, 110, 200, 20, hwnd, nullptr, nullptr, nullptr);
        hBuscarButton = CreateWindowW(L"BUTTON", L"Buscar", WS_VISIBLE | WS_CHILD, 370, 110, 80, 24, hwnd, (HMENU)3, nullptr, nullptr);

        // ListView para resultados de búsqueda
        hResultadoListView = CreateWindowW(WC_LISTVIEWW, L"", WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL | WS_BORDER, 20, 140, 540, 150, hwnd, nullptr, nullptr, nullptr);

        // Configurar columnas del ListView
        LVCOLUMN lvc = {0};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;

        lvc.pszText = L"Título";
        lvc.cx = 200;
        ListView_InsertColumn(hResultadoListView, 0, &lvc);

        lvc.pszText = L"ISBN";
        lvc.cx = 150;
        ListView_InsertColumn(hResultadoListView, 1, &lvc);

        lvc.pszText = L"Estado";
        lvc.cx = 150;
        ListView_InsertColumn(hResultadoListView, 2, &lvc);

        const int BUTTON_WIDTH = 150;
        const int BUTTON_HEIGHT = 30;
        const int BUTTON_Y = 300;
        const int TOTAL_BUTTONS_WIDTH = 2 * BUTTON_WIDTH + 20;
        const int START_X = (575 - TOTAL_BUTTONS_WIDTH) / 2;

        CreateWindowW(L"BUTTON", L"Regresar", WS_VISIBLE | WS_CHILD, START_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)2, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Prestar Libro", WS_VISIBLE | WS_CHILD, START_X + BUTTON_WIDTH + 20, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)1, nullptr, nullptr);
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1)
        {
            wchar_t isbnBuffer[256];
            GetWindowTextW(hIsbnField, isbnBuffer, 256);
            wstring isbnW(isbnBuffer);

            string isbn = ProcesarISBN(hwnd, isbnW);
            if (isbn.empty())
                break;

            wchar_t fechaBuffer[256];
            GetWindowTextW(hFechaDevolucionField, fechaBuffer, 256);
            string fechaDevolucion = WStringToString(fechaBuffer);

            if (!ValidarFechaDevolucion(hwnd, fechaDevolucion))
                break;

            string fechaPostgres = ConvertirFechaFormatoPostgres(fechaDevolucion);
            if (fechaPostgres.empty())
            {
                MessageBoxW(hwnd, L"Error al convertir la fecha", L"Error", MB_ICONERROR);
                break;
            }

            wchar_t lectorBuffer[256];
            GetWindowTextW(hUsernameLectorField, lectorBuffer, 256);
            string lectorIdentificador = WStringToString(lectorBuffer);

            if (lectorIdentificador.empty())
            {
                MessageBoxW(hwnd, L"Ingrese el username del lector.", L"Error", MB_ICONERROR);
                break;
            }

            PGconn *conn = conectarDB();
            if (PQstatus(conn) != CONNECTION_OK)
            {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error de conexión", MB_ICONERROR);
                PQfinish(conn);
                break;
            }

            string bibliotecarioUsername = WStringToString(currentUser);
            const char *biblioParams[1] = {bibliotecarioUsername.c_str()};

            PGresult *res = PQexecParams(conn, "SELECT id FROM usuarios WHERE email = $1 AND (rol = 'bibliotecario' OR rol = 'Admin')", 1, nullptr, biblioParams, nullptr, nullptr, 0);

            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                MessageBoxW(hwnd, L"Error: El usuario actual no es un bibliotecario válido.", L"Error", MB_ICONERROR);
                PQclear(res);
                PQfinish(conn);
                break;
            }

            int bibliotecarioId = atoi(PQgetvalue(res, 0, 0));
            PQclear(res);

            const char *lectorParams[1] = {lectorIdentificador.c_str()};
            res = PQexecParams(conn, "SELECT id, email FROM usuarios WHERE (username = $1 OR email = $1) AND rol = 'Lector'", 1, nullptr, lectorParams, nullptr, nullptr, 0);

            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                MessageBoxW(hwnd, L"Lector no encontrado (por username/email) o no tiene rol 'lector'.", L"Error", MB_ICONERROR);
                PQclear(res);
                PQfinish(conn);
                break;
            }

            int lectorId = atoi(PQgetvalue(res, 0, 0));
            string lectorEmail = PQgetvalue(res, 0, 1);
            PQclear(res);

            PGresult *resLibro = PQexecParams(conn, "SELECT id, titulo, estado FROM libros WHERE REGEXP_REPLACE(isbn, '[^0-9]', '', 'g') = $1", 1, nullptr, (const char *[]){isbn.c_str()}, nullptr, nullptr, 0);

            if (PQresultStatus(resLibro) != PGRES_TUPLES_OK || PQntuples(resLibro) == 0)
            {
                MessageBoxW(hwnd, L"Libro no encontrado", L"Error", MB_ICONERROR);
                PQclear(resLibro);
                PQfinish(conn);
                break;
            }

            int libroId = atoi(PQgetvalue(resLibro, 0, 0));
            string titulo = PQgetvalue(resLibro, 0, 1);
            string estado = PQgetvalue(resLibro, 0, 2);
            PQclear(resLibro);

            if (estado != "Disponible")
            {
                MessageBoxW(hwnd, L"El libro no está disponible", L"Error", MB_ICONERROR);
                PQfinish(conn);
                break;
            }

            const char *prestamoParams[4] = {
                to_string(lectorId).c_str(),
                to_string(libroId).c_str(),
                fechaPostgres.c_str(),
                to_string(bibliotecarioId).c_str()};

            PGresult *insertRes = PQexecParams(conn,
                                               "INSERT INTO prestamos (usuario_id, libro_id, fecha_devolucion, id_bibliotecario) VALUES ($1::int, $2::int, $3::date, $4::int)",
                                               4, nullptr, prestamoParams, nullptr, nullptr, 0);

            if (PQresultStatus(insertRes) != PGRES_COMMAND_OK)
            {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error al registrar préstamo", MB_ICONERROR);
                PQclear(insertRes);
                PQfinish(conn);
                break;
            }

            PQclear(insertRes);

            PGresult *updateRes = PQexecParams(conn,
                                               "UPDATE libros SET estado = 'Prestado' WHERE id = $1",
                                               1, nullptr, (const char *[]){to_string(libroId).c_str()}, nullptr, nullptr, 0);

            if (PQresultStatus(updateRes) != PGRES_COMMAND_OK)
            {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error al actualizar estado del libro", MB_ICONERROR);
                PQclear(updateRes);
                PQfinish(conn);
                break;
            }

            PQclear(updateRes);
            PQfinish(conn);

            // Envío de correo de confirmación (siempre se envía)
            try {
                if (!lectorEmail.empty()) {
                    string asunto = "Confirmación de Préstamo de Libro";
                    string cuerpo = "Estimado lector,\n\nSe ha registrado un préstamo a su nombre:\n";
                    cuerpo += "Libro: " + titulo + "\nISBN: " + isbn + "\n";
                    cuerpo += "Fecha devolución: " + fechaDevolucion + "\n\n";
                    cuerpo += "Por favor devuelva el libro a tiempo.\n\nGracias.\nBiblioteca.";
                    
                    EmailSender::sendEmail(lectorEmail, asunto, cuerpo);
                    cout << "[INFO] Correo de confirmación enviado a " << lectorEmail << endl;
                }
            } catch (const exception& e) {
                cerr << "[ERROR] Envío de confirmación: " << e.what() << endl;
            }

            // Verificación para recordatorio (solo si la devolución es mañana)
            time_t tActual = time(nullptr);
            tm* tmActual = localtime(&tActual);
            
            tm fechaDev = {};
            if (sscanf(fechaDevolucion.c_str(), "%d-%d-%d", &fechaDev.tm_mday, &fechaDev.tm_mon, &fechaDev.tm_year) == 3) {
                fechaDev.tm_mon -= 1;
                fechaDev.tm_year -= 1900;
                time_t tDev = mktime(&fechaDev);
                
                // Calcular fecha de mañana
                tm tmManana = *tmActual;
                tmManana.tm_mday += 1;
                mktime(&tmManana);
                
                // Comparar con fecha de devolución
                tm* tmDev = localtime(&tDev);
                if (tmManana.tm_year == tmDev->tm_year &&
                    tmManana.tm_mon == tmDev->tm_mon &&
                    tmManana.tm_mday == tmDev->tm_mday) {
                    try {
                        if (!lectorEmail.empty()) {
                            EmailSender::sendReminderEmail(lectorEmail, titulo, fechaDevolucion);
                            cout << "[INFO] Correo recordatorio enviado a " << lectorEmail << endl;
                        }
                    } catch (const exception& e) {
                        cerr << "[ERROR] Envío de recordatorio: " << e.what() << endl;
                    }
                }
            }

            wstring mensaje = L"Préstamo registrado exitosamente\n";
            mensaje += L"Libro: " + StringToWString(titulo) + L"\n";
            mensaje += L"Fecha devolución: " + StringToWString(fechaDevolucion);
            MessageBoxW(hwnd, mensaje.c_str(), L"Éxito", MB_OK);
            SetWindowTextW(hIsbnField, L"978-");
            SetWindowTextW(hFechaDevolucionField, L"");
            SetWindowTextW(hUsernameLectorField, L"");
            break;
        }
        else if (LOWORD(wParam) == 2)
        {
            DestroyWindow(hwnd);
            ShowWindow(hMenuWindow, SW_SHOW);
        }
        else if (LOWORD(wParam) == 3) // Acción del botón Buscar
        {
            wchar_t tituloBuffer[256];
            GetWindowTextW(hTituloField, tituloBuffer, 256);
            string tituloBusqueda = WStringToString(tituloBuffer);

            if (tituloBusqueda.empty())
            {
                MessageBoxW(hwnd, L"Ingrese un título para buscar", L"Advertencia", MB_ICONWARNING);
                break;
            }

            PGconn *conn = conectarDB();
            if (PQstatus(conn) != CONNECTION_OK)
            {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error de conexión", MB_ICONERROR);
                PQfinish(conn);
                break;
            }

            string consulta = "%" + tituloBusqueda + "%";
            const char *params[1] = {consulta.c_str()};
            PGresult *res = PQexecParams(conn, "SELECT titulo, isbn, estado FROM libros WHERE LOWER(titulo) LIKE LOWER($1)", 1, nullptr, params, nullptr, nullptr, 0);

            if (PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error al buscar libros", MB_ICONERROR);
                PQclear(res);
                PQfinish(conn);
                break;
            }

            ListView_DeleteAllItems(hResultadoListView);

            int filas = PQntuples(res);
            for (int i = 0; i < filas; ++i)
            {
                string titulo = PQgetvalue(res, i, 0);
                string isbn = PQgetvalue(res, i, 1);
                string estado = PQgetvalue(res, i, 2);

                LVITEM item = {0};
                item.mask = LVIF_TEXT;
                item.iItem = i;
                static wstring wsTitulo;
                wsTitulo = StringToWString(titulo);
                item.pszText = &wsTitulo[0];
                ListView_InsertItem(hResultadoListView, &item);

                static wstring wsIsbn, wsEstado;
                wsIsbn = StringToWString(isbn);
                wsEstado = StringToWString(estado);

                ListView_SetItemText(hResultadoListView, i, 1, &wsIsbn[0]);
                ListView_SetItemText(hResultadoListView, i, 2, &wsEstado[0]);
            }

            PQclear(res);
            PQfinish(conn);
            break;
        }
        break;

    case WM_NOTIFY:
    {
        LPNMHDR nmhdr = (LPNMHDR)lParam;
        if (nmhdr->hwndFrom == hResultadoListView && nmhdr->code == NM_DBLCLK)
        {
            LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)lParam;
            if (item->iItem != -1)
            {
                wchar_t isbnBuffer[256];
                ListView_GetItemText(hResultadoListView, item->iItem, 1, isbnBuffer, sizeof(isbnBuffer)/sizeof(wchar_t));
                SetWindowTextW(hIsbnField, isbnBuffer);
                SetFocus(hFechaDevolucionField);
            }
        }
        break;
    }


    case WM_DESTROY:
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void CrearVentanaPrestamo(HINSTANCE hInstance, HWND hWndMenu)
{
    WNDCLASSW wc = {0};
    wc.lpszClassName = L"LoanBookWindowClass";
    wc.hInstance = hInstance;
    wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    wc.lpfnWndProc = LoanBookWndProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

    RegisterClassW(&wc);

    wstring titulo = L"Prestar Libro - Usuario: " + currentUser;

    HWND hwndLoan = CreateWindowW(wc.lpszClassName, titulo.c_str(), WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, nullptr, nullptr, hInstance, nullptr);

    if (hwndLoan == nullptr)
    {
        MessageBoxW(nullptr, L"No se pudo crear la ventana de préstamo", L"Error", MB_ICONERROR);
        return;
    }

    WindowUtils::CenterWindow(hwndLoan);

    hMenuWindow = hWndMenu;
    ShowWindow(hMenuWindow, SW_HIDE);
    ShowWindow(hwndLoan, SW_SHOW);
    UpdateWindow(hwndLoan);
}

void ShowLoanBookWindow(HINSTANCE hInstance, const wstring &username, HWND hWndMenu)
{
    currentUser = username;
    CrearVentanaPrestamo(hInstance, hWndMenu);
}