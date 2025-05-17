#include <windows.h>
#include <string>
#include <libpq-fe.h>
#include <commctrl.h>
#include <stdexcept>

#include "resources.h"
#include "ListarLibrosWindow.h"
#include "StringUtils.h"
#include "MenuWindow.h"

#pragma comment(lib, "comctl32.lib")

LRESULT CALLBACK ListarLibrosWndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE gListarInst;
std::wstring gListarUsername;

enum class OrdenLibros {
    TITULO,
    AUTOR
};

void CargarLibrosDesdeDB(HWND hwnd, OrdenLibros orden);

void ShowListarLibrosWindow(HINSTANCE hInstance, const std::wstring& username)
{
    gListarInst = hInstance;
    gListarUsername = username;

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = ListarLibrosWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ListarLibrosWindow";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    std::wstring title = L"Lista de Libros - Usuario: " + username;
    HWND hwnd = CreateWindowW(L"ListarLibrosWindow", title.c_str(), WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 800, 500, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Aquí agregamos el parámetro orden para modificar la consulta según criterio
void CargarLibrosDesdeDB(HWND hwnd, OrdenLibros orden)
{
    PGconn* conn = PQconnectdb("host=localhost dbname=postgres user=postgres password=Myroot");

    if (!conn || PQstatus(conn) != CONNECTION_OK)
    {
        std::wstring werr = Utf8ToWstring(PQerrorMessage(conn));
        MessageBoxW(hwnd, (L"Error al conectar:\n" + werr).c_str(), L"Error", MB_OK | MB_ICONERROR);
        if (conn) PQfinish(conn);
        return;
    }

    // Construimos la consulta con ORDER BY según orden seleccionado
    const char* queryBase = "SELECT titulo, autor, isbn, editorial, año, materia, estado FROM libros ORDER BY ";
    std::string query;

    if (orden == OrdenLibros::TITULO) {
        query = std::string(queryBase) + "titulo ASC;";
    }
    else { // AUTOR
        query = std::string(queryBase) + "autor ASC;";
    }

    PGresult* res = PQexec(conn, query.c_str());

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::wstring werr = Utf8ToWstring(PQresultErrorMessage(res));
        MessageBoxW(hwnd, (L"Error en la consulta:\n" + werr).c_str(), L"Consulta fallida", MB_OK | MB_ICONERROR);
        if (res) PQclear(res);
        PQfinish(conn);
        return;
    }

    HWND hListView = GetDlgItem(hwnd, 10);
    ListView_DeleteAllItems(hListView);

    int rows = PQntuples(res);
    if (rows == 0)
    {
        MessageBoxW(hwnd, L"No hay libros registrados en la biblioteca.", L"Información", MB_OK | MB_ICONINFORMATION);
    }

    for (int i = 0; i < rows; ++i)
    {
        std::wstring wTitulo    = Utf8ToWstring(PQgetvalue(res, i, 0));
        std::wstring wAutor     = Utf8ToWstring(PQgetvalue(res, i, 1));
        std::wstring wIsbn      = Utf8ToWstring(PQgetvalue(res, i, 2));
        std::wstring wEditorial = Utf8ToWstring(PQgetvalue(res, i, 3));
        std::wstring wAnio      = Utf8ToWstring(PQgetvalue(res, i, 4));
        std::wstring wMateria   = Utf8ToWstring(PQgetvalue(res, i, 5));
        std::wstring wEstado    = Utf8ToWstring(PQgetvalue(res, i, 6));

        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.pszText = (LPWSTR)wTitulo.c_str();
        ListView_InsertItem(hListView, &item);

        ListView_SetItemText(hListView, i, 1, (LPWSTR)wAutor.c_str());
        ListView_SetItemText(hListView, i, 2, (LPWSTR)wIsbn.c_str());
        ListView_SetItemText(hListView, i, 3, (LPWSTR)wEditorial.c_str());
        ListView_SetItemText(hListView, i, 4, (LPWSTR)wAnio.c_str());
        ListView_SetItemText(hListView, i, 5, (LPWSTR)wMateria.c_str());
        ListView_SetItemText(hListView, i, 6, (LPWSTR)wEstado.c_str());
    }

    PQclear(res);
    PQfinish(conn);
}

LRESULT CALLBACK ListarLibrosWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static OrdenLibros ordenActual = OrdenLibros::TITULO; // Por defecto ordenamos por título

    switch (msg)
    {
    case WM_CREATE:
    {
        HWND hListView = CreateWindowW(WC_LISTVIEW, NULL,
            WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
            20, 20, 740, 330, hwnd, (HMENU)10, gListarInst, NULL);

        LVCOLUMNW lvc = { LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM };

        const wchar_t* headers[] = { L"Título", L"Autor", L"ISBN", L"Editorial", L"Año", L"Materia", L"Estado" };
        int widths[] = { 120, 100, 120, 100, 50, 100, 100 };

        for (int i = 0; i < 7; ++i)
        {
            lvc.pszText = (LPWSTR)headers[i];
            lvc.cx = widths[i];
            lvc.iSubItem = i;
            ListView_InsertColumn(hListView, i, &lvc);
        }

        // Botones para ordenar
        CreateWindowW(L"BUTTON", L"Ordenar por título", WS_VISIBLE | WS_CHILD,
                      20, 370, 140, 30, hwnd, (HMENU)3, gListarInst, NULL);

        CreateWindowW(L"BUTTON", L"Ordenar por autor", WS_VISIBLE | WS_CHILD,
                      180, 370, 140, 30, hwnd, (HMENU)4, gListarInst, NULL);

        // Botones actualizar y regresar
        CreateWindowW(L"BUTTON", L"Actualizar", WS_VISIBLE | WS_CHILD,
                      20, 410, 100, 30, hwnd, (HMENU)1, gListarInst, NULL);

        CreateWindowW(L"BUTTON", L"Regresar", WS_VISIBLE | WS_CHILD,
                      660, 410, 100, 30, hwnd, (HMENU)2, gListarInst, NULL);

        // Carga inicial ordenada por título
        CargarLibrosDesdeDB(hwnd, ordenActual);
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1: // Actualizar con orden actual
            CargarLibrosDesdeDB(hwnd, ordenActual);
            break;

        case 2: // Regresar
            DestroyWindow(hwnd);
            ShowMenuWindow(gListarInst, gListarUsername);
            break;

        case 3: // Ordenar por título
            ordenActual = OrdenLibros::TITULO;
            CargarLibrosDesdeDB(hwnd, ordenActual);
            break;

        case 4: // Ordenar por autor
            ordenActual = OrdenLibros::AUTOR;
            CargarLibrosDesdeDB(hwnd, ordenActual);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
