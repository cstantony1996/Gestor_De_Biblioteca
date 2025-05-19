#include "byte_fix.h" 
#include "resources.h"
#include "WindowUtils.h"
#include "MenuWindow.h"
#include "BuscarLibro.h"
#include "StringUtils.h"
#include <string>
#include <libpq-fe.h>
#include <commctrl.h>
#include <stdexcept>
#pragma comment(lib, "comctl32.lib")

using namespace std;

LRESULT CALLBACK BuscarLibroWndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE gBuscarInst;
wstring gBuscarUsername;

// Ya NO defines Utf8ToWstring aquí, se usa la que está en StringUtils.cpp

void ShowBuscarLibroWindow(HINSTANCE hInstance, const wstring &username)
{
    gBuscarInst = hInstance;
    gBuscarUsername = username;

    INITCOMMONCONTROLSEX icex = {sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES};
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = BuscarLibroWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"BuscarLibroWindow";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); // Icono principal
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);                   // Opcional
    RegisterClassW(&wc);

    wstring windowTitle = L"Buscar Libro - Usuario: " + gBuscarUsername;

    HWND hwnd = CreateWindowW(L"BuscarLibroWindow", windowTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 500, NULL, NULL, hInstance, NULL);

    WindowUtils::CenterWindow(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void BuscarLibroEnDB(HWND hwnd)
{
    try
    {
        wchar_t buffer[256];
        GetWindowTextW(GetDlgItem(hwnd, 1), buffer, 256);
        wstring textoW(buffer);

        if (textoW.empty())
        {
            MessageBoxW(hwnd, L"Por favor, introduzca un criterio de búsqueda.", L"Campo vacío", MB_OK | MB_ICONWARNING);
            return;
        }

        string texto = WStringToString(textoW); // mejor usar la función de StringUtils

        PGconn *conn = PQconnectdb("host=localhost dbname=postgres user=postgres password=Myroot");

        if (!conn)
        {
            MessageBoxW(hwnd, L"Error al crear conexión con PostgreSQL.", L"Conexión fallida", MB_OK | MB_ICONERROR);
            return;
        }

        if (PQstatus(conn) != CONNECTION_OK)
        {
            wstring werr = Utf8ToWstring(PQerrorMessage(conn));
            MessageBoxW(hwnd, (L"Conexión fallida:\n" + werr).c_str(), L"Error de conexión", MB_OK | MB_ICONERROR);
            PQfinish(conn);
            return;
        }

        int criterio;
        if (IsDlgButtonChecked(hwnd, 4))
            criterio = 4;
        else if (IsDlgButtonChecked(hwnd, 5))
            criterio = 5;
        else
            criterio = 6;

        string query;
        if (criterio == 4)
            query = "SELECT titulo, autor, isbn, editorial, año, materia, estado FROM libros WHERE titulo ILIKE '%" + texto + "%';";
        else if (criterio == 5)
            query = "SELECT titulo, autor, isbn, editorial, año, materia, estado FROM libros WHERE autor ILIKE '%" + texto + "%';";
        else
            query = "SELECT titulo, autor, isbn, editorial, año, materia, estado FROM libros WHERE titulo ILIKE '%" + texto + "%' OR autor ILIKE '%" + texto + "%';";

        PGresult *res = PQexec(conn, query.c_str());

        if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            wstring werr = Utf8ToWstring(PQresultErrorMessage(res));
            MessageBoxW(hwnd, (L"Error en la consulta:\n" + werr).c_str(), L"Error de consulta", MB_OK | MB_ICONERROR);
            if (res)
                PQclear(res);
            PQfinish(conn);
            return;
        }

        HWND hListView = GetDlgItem(hwnd, 10);
        if (!hListView)
        {
            MessageBoxW(hwnd, L"No se pudo obtener el control ListView.", L"Error de interfaz", MB_OK | MB_ICONERROR);
            PQclear(res);
            PQfinish(conn);
            return;
        }

        ListView_DeleteAllItems(hListView);

        int rows = PQntuples(res);

        if (rows == 0)
        {
            MessageBoxW(hwnd, L"No se encontró información.", L"Sin resultados", MB_OK | MB_ICONINFORMATION);
        }

        for (int i = 0; i < rows; ++i)
        {
            wstring wTitulo = Utf8ToWstring(PQgetvalue(res, i, 0));
            wstring wAutor = Utf8ToWstring(PQgetvalue(res, i, 1));
            wstring wIsbn = Utf8ToWstring(PQgetvalue(res, i, 2));
            wstring wEditorial = Utf8ToWstring(PQgetvalue(res, i, 3));
            wstring wAnio = Utf8ToWstring(PQgetvalue(res, i, 4));
            wstring wMateria = Utf8ToWstring(PQgetvalue(res, i, 5));
            wstring wEstado = Utf8ToWstring(PQgetvalue(res, i, 6));

            LVITEMW lvi = {};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.pszText = (LPWSTR)wTitulo.c_str();
            ListView_InsertItem(hListView, &lvi);

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
    catch (const exception &e)
    {
        wstring msg = L"Excepción: " + Utf8ToWstring(e.what());
        MessageBoxW(hwnd, msg.c_str(), L"Error inesperado", MB_OK | MB_ICONERROR);
    }
    catch (...)
    {
        MessageBoxW(hwnd, L"Ocurrió un error desconocido.", L"Error crítico", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK BuscarLibroWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Ingrese título o autor del libro:", WS_VISIBLE | WS_CHILD,
                      20, 20, 250, 20, hwnd, NULL, gBuscarInst, NULL);

        CreateWindowW(L"EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER,
                      20, 50, 250, 25, hwnd, (HMENU)1, gBuscarInst, NULL);

        CreateWindowW(L"BUTTON", L"Buscar", WS_VISIBLE | WS_CHILD,
                      290, 50, 100, 25, hwnd, (HMENU)2, gBuscarInst, NULL);

        CreateWindowW(L"BUTTON", L"Salir", WS_VISIBLE | WS_CHILD,
                      650, 420, 100, 30, hwnd, (HMENU)3, gBuscarInst, NULL);

        CreateWindowW(L"BUTTON", L"Buscar por título", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                      20, 90, 150, 20, hwnd, (HMENU)4, gBuscarInst, NULL);

        CreateWindowW(L"BUTTON", L"Buscar por autor", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                      20, 120, 150, 20, hwnd, (HMENU)5, gBuscarInst, NULL);

        CreateWindowW(L"BUTTON", L"Buscar por ambos", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                      20, 150, 150, 20, hwnd, (HMENU)6, gBuscarInst, NULL);

        CheckRadioButton(hwnd, 4, 6, 6);

        HWND hListView = CreateWindowW(WC_LISTVIEW, NULL,
                                       WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
                                       20, 200, 740, 200, hwnd, (HMENU)10, gBuscarInst, NULL);

        LVCOLUMNW lvc = {};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        lvc.pszText = (LPWSTR)L"Título";
        lvc.cx = 120;
        ListView_InsertColumn(hListView, 0, &lvc);

        lvc.pszText = (LPWSTR)L"Autor";
        lvc.cx = 100;
        ListView_InsertColumn(hListView, 1, &lvc);

        lvc.pszText = (LPWSTR)L"ISBN";
        lvc.cx = 120;
        ListView_InsertColumn(hListView, 2, &lvc);

        lvc.pszText = (LPWSTR)L"Editorial";
        lvc.cx = 100;
        ListView_InsertColumn(hListView, 3, &lvc);

        lvc.pszText = (LPWSTR)L"Año";
        lvc.cx = 50;
        ListView_InsertColumn(hListView, 4, &lvc);

        lvc.pszText = (LPWSTR)L"Materia";
        lvc.cx = 100;
        ListView_InsertColumn(hListView, 5, &lvc);

        lvc.pszText = (LPWSTR)L"Estado";
        lvc.cx = 100;
        ListView_InsertColumn(hListView, 6, &lvc);
    }
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 2:
            BuscarLibroEnDB(hwnd);
            break;

        case 3:
            DestroyWindow(hwnd);
            ShowMenuWindow(gBuscarInst, gBuscarUsername);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
