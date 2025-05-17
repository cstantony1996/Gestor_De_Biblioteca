#include "ReturnBookWindow.h"
#include "Connection.h"
#include "StringUtils.h"
#include "GlobalVars.h"
#include "MenuWindow.h"
#include "resources.h"
#include <commctrl.h>
#include <libpq-fe.h>
#include <vector>
#include <string>
#include <stdexcept>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "libpq.lib")

static HWND hIsbnField, hListView;
static HWND hMenuWindow = nullptr;

struct Prestamo {
    int id;
    std::wstring titulo;
    std::wstring isbn;
    std::wstring fechaDevolucion;
};


std::vector<Prestamo> prestamosUsuario;

void LlenarListaPrestamos(HWND hwnd, const std::wstring& username) {
    try {
        ListView_DeleteAllItems(hListView);
        prestamosUsuario.clear();

        // Evita agregar columnas más de una vez
        static bool columnasConfiguradas = false;
        if (!columnasConfiguradas) {
            LVCOLUMNW col = {};
            col.mask = LVCF_TEXT | LVCF_WIDTH;

            col.cx = 200;
            col.pszText = L"Título";
            ListView_InsertColumn(hListView, 0, &col);

            col.cx = 150;
            col.pszText = L"ISBN";
            ListView_InsertColumn(hListView, 1, &col);

            col.cx = 130;
            col.pszText = L"Fecha Devolución";
            ListView_InsertColumn(hListView, 2, &col);

            columnasConfiguradas = true;
        }

        PGconn* conn = conectarDB();
        if (!conn || PQstatus(conn) != CONNECTION_OK) {
            if (conn) PQfinish(conn);
            throw std::runtime_error("No se pudo conectar a la base de datos");
        }

        std::string userStr = WStringToString(username);
        PGresult* userRes = PQexecParams(conn,
            "SELECT id FROM usuarios WHERE username = $1",
            1, nullptr, (const char*[]) { userStr.c_str() }, nullptr, nullptr, 0);

        if (PQresultStatus(userRes) != PGRES_TUPLES_OK || PQntuples(userRes) == 0) {
            PQclear(userRes); PQfinish(conn);
            throw std::runtime_error("Usuario no encontrado");
        }

        int userId = atoi(PQgetvalue(userRes, 0, 0));
        PQclear(userRes);

        PGresult* res = PQexecParams(conn,
    "SELECT l.titulo, l.isbn, to_char(p.fecha_devolucion, 'DD-MM-YYYY'), l.id "
    "FROM prestamos p JOIN libros l ON p.libro_id = l.id "
    "WHERE p.usuario_id = $1",
    1, nullptr, (const char*[]) { std::to_string(userId).c_str() }, nullptr, nullptr, 0);
;

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            PQclear(res); PQfinish(conn);
            throw std::runtime_error("Error al obtener préstamos");
        }

        for (int i = 0; i < PQntuples(res); ++i) {
    Prestamo p;
    p.titulo = StringToWString(PQgetvalue(res, i, 0)); // columna 0: titulo
    p.isbn = StringToWString(PQgetvalue(res, i, 1));   // columna 1: isbn (con guiones)
    p.fechaDevolucion = StringToWString(PQgetvalue(res, i, 2)); // columna 2: fecha devolución
    p.id = atoi(PQgetvalue(res, i, 3)); // columna 3: id libro (no se muestra, pero útil internamente)

    prestamosUsuario.push_back(p);

    LVITEMW item = { 0 };
    item.mask = LVIF_TEXT;
    item.iItem = i;
    item.pszText = const_cast<LPWSTR>(p.titulo.c_str());
    ListView_InsertItem(hListView, &item);

    ListView_SetItemText(hListView, i, 1, const_cast<LPWSTR>(p.isbn.c_str()));
    ListView_SetItemText(hListView, i, 2, const_cast<LPWSTR>(p.fechaDevolucion.c_str()));
}


        PQclear(res);
        PQfinish(conn);
    }
    catch (const std::exception& e) {
        std::wstring mensaje = L"Error: ";
        mensaje += StringToWString(e.what());
        MessageBoxW(hwnd, mensaje.c_str(), L"Excepción", MB_ICONERROR);
    }
}


LRESULT CALLBACK ReturnBookWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"Ingrese el ISBN del libro a devolver:",
            WS_VISIBLE | WS_CHILD, 20, 20, 250, 20, hwnd, nullptr, nullptr, nullptr);

        hIsbnField = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            280, 20, 200, 20, hwnd, nullptr, nullptr, nullptr);

        hListView = CreateWindowW(WC_LISTVIEW, L"",
            WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
            20, 60, 460, 150, hwnd, nullptr, nullptr, nullptr);

        LVCOLUMNW col = { 0 };
        col.mask = LVCF_TEXT | LVCF_WIDTH;

        

        CreateWindowW(L"BUTTON", L"Devolver Libro", WS_VISIBLE | WS_CHILD,
            120, 220, 150, 30, hwnd, (HMENU)1, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Regresar", WS_VISIBLE | WS_CHILD,
            280, 220, 150, 30, hwnd, (HMENU)2, nullptr, nullptr);

        LlenarListaPrestamos(hwnd, currentUser);
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // Botón "Devolver Libro"
            wchar_t isbnBuffer[256];
            GetWindowTextW(hIsbnField, isbnBuffer, 256);
            std::wstring isbnW(isbnBuffer);

            if (isbnW.empty()) {
                MessageBoxW(hwnd, L"El ISBN no puede estar vacío", L"Error", MB_ICONERROR);
                break;
            }

            std::string isbnStr = WStringToString(isbnW);

            PGconn* conn = conectarDB();
            if (!conn || PQstatus(conn) != CONNECTION_OK) {
                MessageBoxW(hwnd, L"No se pudo conectar a la base de datos", L"Error", MB_ICONERROR);
                if (conn) PQfinish(conn);
                break;
            }

            // Obtener ID del usuario
            std::string userStr = WStringToString(currentUser);
            PGresult* userRes = PQexecParams(conn,
                "SELECT id FROM usuarios WHERE username = $1",
                1, nullptr, (const char*[]) { userStr.c_str() }, nullptr, nullptr, 0);

            if (PQresultStatus(userRes) != PGRES_TUPLES_OK || PQntuples(userRes) == 0) {
                PQclear(userRes); PQfinish(conn);
                MessageBoxW(hwnd, L"Usuario no encontrado", L"Error", MB_ICONERROR);
                break;
            }

            int userId = atoi(PQgetvalue(userRes, 0, 0));
            PQclear(userRes);

            // Buscar libro con ISBN exacto
            PGresult* libroRes = PQexecParams(conn,
                "SELECT id FROM libros WHERE isbn = $1",
                1, nullptr, (const char*[]) { isbnStr.c_str() }, nullptr, nullptr, 0);

            if (PQntuples(libroRes) == 0) {
                PQclear(libroRes); PQfinish(conn);
                MessageBoxW(hwnd, L"No se encontró el libro con el ISBN ingresado", L"Error", MB_ICONERROR);
                break;
            }

            int libroId = atoi(PQgetvalue(libroRes, 0, 0));
            PQclear(libroRes);

            // Eliminar el préstamo
            PGresult* del = PQexecParams(conn,
                "DELETE FROM prestamos WHERE libro_id = $1 AND usuario_id = $2",
                2, nullptr,
                (const char*[]) { std::to_string(libroId).c_str(), std::to_string(userId).c_str() },
                nullptr, nullptr, 0);

            if (PQresultStatus(del) != PGRES_COMMAND_OK) {
                MessageBoxW(hwnd, L"No se pudo eliminar el préstamo", L"Error", MB_ICONERROR);
                PQclear(del); PQfinish(conn);
                break;
            }

            PQclear(del);

            // Actualizar estado del libro (opcional)
            PGresult* upd = PQexecParams(conn,
                "UPDATE libros SET estado = 'Disponible' WHERE id = $1",
                1, nullptr, (const char*[]) { std::to_string(libroId).c_str() }, nullptr, nullptr, 0);

            if (PQresultStatus(upd) != PGRES_COMMAND_OK) {
                MessageBoxW(hwnd, L"No se pudo actualizar el estado del libro", L"Advertencia", MB_ICONWARNING);
            }

            PQclear(upd);
            PQfinish(conn);

            MessageBoxW(hwnd, L"El libro ha sido devuelto con éxito", L"Éxito", MB_OK);
            LlenarListaPrestamos(hwnd, currentUser);
        }
        else if (LOWORD(wParam) == 2) { // Botón "Regresar"
            DestroyWindow(hwnd);
            ShowMenuWindow(GetModuleHandle(nullptr), currentUser);
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


void ShowReturnBookWindow(HINSTANCE hInstance, const std::wstring& username, HWND hWndMenu) {
    currentUser = username;
    hMenuWindow = hWndMenu;

    WNDCLASSW wc = { 0 };
    wc.lpszClassName = L"ReturnBookWindowClass";
    wc.hInstance = hInstance;
    wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    wc.lpfnWndProc = ReturnBookWndProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

    RegisterClassW(&wc);

    std::wstring titulo = L"Devolver Libro - Usuario: " + username;

    HWND hwnd = CreateWindowW(wc.lpszClassName, titulo.c_str(),
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 310,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}
