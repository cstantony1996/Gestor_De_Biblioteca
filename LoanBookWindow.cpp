#include "LoanBookWindow.h"
#include "Connection.h"
#include "MenuWindow.h"
#include "StringUtils.h"
#include "GlobalVars.h"
#include <commctrl.h>
#include <string>
#include <libpq-fe.h>
#include <ctime>
#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "libpq.lib")

// Variables de la ventana
HWND hIsbnField;
HWND hFechaDevolucionField;

// Definiciones de IDs para controles
#define IDC_LIST_LOANS 1001
#define IDC_BUTTON_RETURN 1002

// Función para validar fecha de devolución
bool ValidarFechaDevolucion(HWND hwnd, const std::string& fechaStr) {
    // Obtener fecha actual
    time_t ahora = time(nullptr);
    tm* fechaActual = localtime(&ahora);
    
    // Parsear fecha ingresada (formato YYYY-MM-DD)
    int año, mes, dia;
    if (sscanf(fechaStr.c_str(), "%d-%d-%d", &año, &mes, &dia) != 3) {
        MessageBoxW(hwnd, L"Formato de fecha inválido. Use YYYY-MM-DD", L"Error", MB_ICONERROR);
        return false;
    }

    // Crear estructura tm para la fecha de devolución
    tm fechaDevolucion = {0};
    fechaDevolucion.tm_year = año - 1900;
    fechaDevolucion.tm_mon = mes - 1;
    fechaDevolucion.tm_mday = dia;

    // Convertir a time_t para comparación
    time_t tDevolucion = mktime(&fechaDevolucion);
    time_t tActual = mktime(fechaActual);
    
    if (tDevolucion < tActual) {
        MessageBoxW(hwnd, L"La fecha de devolución no puede ser anterior a hoy", L"Error", MB_ICONERROR);
        return false;
    }
    return true;
}

// Función para validar y normalizar ISBN
std::string ProcesarISBN(HWND hwnd, const std::wstring& isbnW) {
    std::wstring cleanIsbn;
    for (wchar_t c : isbnW) {
        if (iswdigit(c)) {
            cleanIsbn += c;
        }
    }
    
    if (cleanIsbn.empty()) {
        MessageBoxW(hwnd, L"El ISBN no puede estar vacío", L"Error", MB_ICONERROR);
        return "";
    }
    
    if (cleanIsbn.length() < 10) {
        MessageBoxW(hwnd, L"ISBN demasiado corto. Debe tener al menos 10 dígitos.", L"Error", MB_ICONERROR);
        return "";
    }
    
    return WStringToString(cleanIsbn);
}

LRESULT CALLBACK LoanBookWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Campo para ISBN
        CreateWindowW(L"STATIC", L"Ingrese el ISBN del libro a prestar:", 
                     WS_VISIBLE | WS_CHILD, 20, 20, 280, 20, hwnd, nullptr, nullptr, nullptr);

        hIsbnField = CreateWindowW(L"EDIT", L"978-", 
                                  WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                  300, 20, 200, 20, hwnd, nullptr, nullptr, nullptr);

        // Campo para fecha de devolución
        CreateWindowW(L"STATIC", L"Fecha de devolución (YYYY-MM-DD):", 
                     WS_VISIBLE | WS_CHILD, 20, 50, 280, 20, hwnd, nullptr, nullptr, nullptr);

        hFechaDevolucionField = CreateWindowW(L"EDIT", L"", 
                                  WS_VISIBLE | WS_CHILD | WS_BORDER,
                                  300, 50, 200, 20, hwnd, nullptr, nullptr, nullptr);

        // Botones
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
        if (LOWORD(wParam) == 1) { // Prestar libro
            // Obtener ISBN
            wchar_t isbnBuffer[256];
            GetWindowTextW(hIsbnField, isbnBuffer, 256);
            std::wstring isbnW(isbnBuffer);
            
            // Validar y normalizar ISBN
            std::string isbn = ProcesarISBN(hwnd, isbnW);
            if (isbn.empty()) break;
            
            // Obtener fecha de devolución
            wchar_t fechaBuffer[256];
            GetWindowTextW(hFechaDevolucionField, fechaBuffer, 256);
            std::string fechaDevolucion = WStringToString(fechaBuffer);
            
            // Validar fecha
            if (!ValidarFechaDevolucion(hwnd, fechaDevolucion)) {
                break;
            }
            
            PGconn* conn = conectarDB();
            if (PQstatus(conn) != CONNECTION_OK) {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error de conexión", MB_ICONERROR);
                PQfinish(conn);
                break;
            }

            // Buscar libro por ISBN normalizado
            const char* paramValues[1] = { isbn.c_str() };
            PGresult* res = PQexecParams(conn,
                "SELECT titulo, estado FROM libros WHERE REGEXP_REPLACE(isbn, '[^0-9]', '', 'g') = $1",
                1, nullptr, paramValues, nullptr, nullptr, 0);

            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
                MessageBoxW(hwnd, L"Libro no encontrado. Verifique el ISBN.", L"Error", MB_ICONERROR);
                PQclear(res);
                PQfinish(conn);
                break;
            }

            std::string tituloLibro = PQgetvalue(res, 0, 0);
            std::string estado = PQgetvalue(res, 0, 1);
            PQclear(res);

            if (estado != "Disponible") {
                MessageBoxW(hwnd, L"El libro no está disponible para préstamo.", L"Error", MB_ICONERROR);
                PQfinish(conn);
                break;
            }

            // Registrar préstamo
            std::string username = WStringToString(currentUser);
            
            const char* insertParams[5] = { 
                isbn.c_str(),
                tituloLibro.c_str(),
                username.c_str(),
                "now()",
                fechaDevolucion.c_str()
            };
            
            res = PQexecParams(conn,
                "INSERT INTO prestamos (isbn, titulo, usuario, fecha_prestamo, fecha_devolucion) "
                "VALUES ($1, $2, $3, $4::timestamp, $5::date)",
                5, nullptr, insertParams, nullptr, nullptr, 0);

            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error al registrar préstamo", MB_ICONERROR);
                PQclear(res);
                PQfinish(conn);
                break;
            }
            PQclear(res);

            // Actualizar estado del libro
            res = PQexecParams(conn,
                "UPDATE libros SET estado = 'Prestado' WHERE REGEXP_REPLACE(isbn, '[^0-9]', '', 'g') = $1",
                1, nullptr, paramValues, nullptr, nullptr, 0);

            if (PQresultStatus(res) == PGRES_COMMAND_OK) {
                std::wstring mensaje = L"Préstamo registrado exitosamente\n";
                mensaje += L"Libro: " + StringToWString(tituloLibro) + L"\n";
                mensaje += L"Fecha devolución: " + StringToWString(fechaDevolucion);
                MessageBoxW(hwnd, mensaje.c_str(), L"Éxito", MB_OK);
                SetWindowTextW(hIsbnField, L"978-");
                SetWindowTextW(hFechaDevolucionField, L"");
            } else {
                MessageBoxA(hwnd, PQerrorMessage(conn), "Error al actualizar estado", MB_ICONERROR);
            }

            PQclear(res);
            PQfinish(conn);
        } 
        else if (LOWORD(wParam) == 2) { // Regresar
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

LRESULT CALLBACK ActiveLoansWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Crear lista de préstamos activos
            HWND hList = CreateWindowW(WC_LISTVIEWW, L"", 
                WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
                10, 10, 760, 500, hwnd, (HMENU)IDC_LIST_LOANS, 
                GetModuleHandle(nullptr), nullptr);
            
            // Configurar columnas
            LVCOLUMNW lvc = {};
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            
            // Columnas: ISBN, Título, Fecha Préstamo, Fecha Devolución
            const wchar_t* headers[] = {L"ISBN", L"Título", L"Fecha Préstamo", L"Fecha Devolución"};
            int widths[] = {120, 300, 150, 150};
            
            for (int i = 0; i < 4; i++) {
                lvc.iSubItem = i;
                lvc.pszText = const_cast<wchar_t*>(headers[i]);
                lvc.cx = widths[i];
                lvc.fmt = LVCFMT_LEFT;
                ListView_InsertColumn(hList, i, &lvc);
            }
            
            // Cargar datos de la base de datos
            PGconn* conn = conectarDB();
            if (PQstatus(conn) == CONNECTION_OK) {
                std::string username = WStringToString(currentUser);
                const char* paramValues[1] = {username.c_str()};
                
                PGresult* res = PQexecParams(conn,
                    "SELECT isbn, titulo, "
                    "to_char(fecha_prestamo, 'DD/MM/YYYY'), "
                    "to_char(fecha_devolucion, 'DD/MM/YYYY') "
                    "FROM prestamos "
                    "WHERE usuario = $1 AND fecha_devolucion >= CURRENT_DATE "
                    "ORDER BY fecha_devolucion ASC",
                    1, nullptr, paramValues, nullptr, nullptr, 0);
                
                if (PQresultStatus(res) == PGRES_TUPLES_OK) {
                    for (int i = 0; i < PQntuples(res); i++) {
                        LVITEMW lvi = {0};
                        lvi.mask = LVIF_TEXT;
                        lvi.iItem = i;
                        lvi.iSubItem = 0;
                        std::wstring texto = StringToWString(PQgetvalue(res, i, 0));
                        lvi.pszText = const_cast<wchar_t*>(texto.c_str());
                        ListView_InsertItem(hList, &lvi);
                        
                        texto = StringToWString(PQgetvalue(res, i, 1));
                        ListView_SetItemText(hList, i, 1, const_cast<wchar_t*>(texto.c_str()));
                        
                        texto = StringToWString(PQgetvalue(res, i, 2));
                        ListView_SetItemText(hList, i, 2, const_cast<wchar_t*>(texto.c_str()));
                        
                        texto = StringToWString(PQgetvalue(res, i, 3));
                        ListView_SetItemText(hList, i, 3, const_cast<wchar_t*>(texto.c_str()));
                    }
                }
                PQclear(res);
                PQfinish(conn);
            }
            break;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON_RETURN) {
                HWND hList = GetDlgItem(hwnd, IDC_LIST_LOANS);
                int selected = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                
                if (selected != -1) {
                    wchar_t isbn[256] = {0};
                    ListView_GetItemText(hList, selected, 0, isbn, 256);
                    
                    // Aquí iría la lógica para devolver el libro
                    MessageBoxW(hwnd, L"Función de devolución en desarrollo", L"Información", MB_OK);
                }
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowActiveLoansWindow(HINSTANCE hInstance, const std::wstring& username) {
    currentUser = username;
    
    const wchar_t CLASS_NAME[] = L"ActiveLoansWindow";
    
    WNDCLASSW wc = {};
    wc.lpfnWndProc = ActiveLoansWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    
    RegisterClassW(&wc);
    
    std::wstring title = L"Préstamos Activos - Usuario: " + username;
    
    HWND hwnd = CreateWindowW(
        CLASS_NAME,
        title.c_str(),
        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr, nullptr, hInstance, nullptr);
    
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

void ShowLoanBookWindow(HINSTANCE hInstance, const std::wstring& username) {
    currentUser = username;

    const wchar_t CLASS_NAME[] = L"LoanBookWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = LoanBookWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    std::wstring title = L"Préstamo de Libros - Usuario: " + username;

    HWND hwnd = CreateWindowW(
        CLASS_NAME,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        575, 180,  // Ajustado para el nuevo campo
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}