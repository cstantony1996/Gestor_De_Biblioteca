#include "byte_fix.h" 
#include "MenuWindow.h"
#include "LoginWindow.h"
#include "AddBookWindow.h"
#include "LoanBookWindow.h"
#include "BuscarLibro.h"
#include "ReturnBookWindow.h"
#include "ListarLibrosWindow.h"
#include "RegisterUserWindow.h"
#include "WindowUtils.h"
#include "resources.h"
#include "GlobalVars.h"
#include "EmailReminder.h"
#include <string>

using namespace std;

LRESULT CALLBACK MenuWndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE gInst;
wstring gUsername;


void ShowMenuWindow(HINSTANCE hInstance, const wstring &username, const wstring &userRole)
{
    gInst = hInstance;
    gUsername = username;

    WNDCLASSW wc = {};
    wc.lpfnWndProc = MenuWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MenuWindow";
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    RegisterClassW(&wc);

    wstring windowTitle = L"Menú Principal - Usuario: " + username;

    HWND hwnd = CreateWindowW(L"MenuWindow", windowTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 430, 450, NULL, NULL, hInstance, NULL);

    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(gInst, MAKEINTRESOURCE(IDI_ICON1)));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL,(LPARAM)LoadImage(gInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

    WindowUtils::CenterWindow(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // ✅ Bucle de mensajes necesario para mantener abierta la ventana
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:

    RevisarYEnviarRecordatorios();


        CreateWindowW(L"BUTTON", L"Agregar Libro", WS_VISIBLE | WS_CHILD, 115, 50, 200, 40, hwnd, (HMENU)1, gInst, NULL);
        CreateWindowW(L"BUTTON", L"Buscar Libro", WS_VISIBLE | WS_CHILD, 115, 100, 200, 40, hwnd, (HMENU)2, gInst, NULL);
        CreateWindowW(L"BUTTON", L"Listar Libros", WS_VISIBLE | WS_CHILD, 115, 150, 200, 40, hwnd, (HMENU)3, gInst, NULL);
        CreateWindowW(L"BUTTON", L"Prestar Libros", WS_VISIBLE | WS_CHILD, 115, 200, 200, 40, hwnd, (HMENU)4, gInst, NULL);
        CreateWindowW(L"BUTTON", L"Devolver Libros", WS_VISIBLE | WS_CHILD, 115, 250, 200, 40, hwnd, (HMENU)5, gInst, NULL);
        CreateWindowW(L"BUTTON", L"Cerrar Sesión", WS_VISIBLE | WS_CHILD, 125, 320, 180, 20, hwnd, (HMENU)6, NULL, NULL);
        
        if (currentRole == L"Admin") {
            CreateWindowW(L"BUTTON", L"Registrar", WS_VISIBLE | WS_CHILD, 315, 10, 90, 30, hwnd, (HMENU)7, NULL, NULL);
        }
        
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1:
            DestroyWindow(hwnd);
            ShowAddBookWindow(gInst, gUsername);
            break;
        case 2:
            DestroyWindow(hwnd);
            ShowBuscarLibroWindow(gInst, gUsername, currentRole);
            break;
        case 3:
            DestroyWindow(hwnd);
            ShowListarLibrosWindow(gInst, gUsername);
            break;
        case 4:
            ShowLoanBookWindow(gInst, gUsername, hwnd); // pasa hwnd como ventana padre
            ShowWindow(hwnd, SW_HIDE);
            break;
        case 5:
            DestroyWindow(hwnd); // Oculta el menú
            ShowReturnBookWindow(gInst, gUsername, hwnd);
            break;
        case 6:                     // ✅ Cerrar Sesión
            DestroyWindow(hwnd);    // Cierra la ventana de menú
            ShowLoginWindow(gInst); // Vuelve a mostrar el login
            return 0;
        case 7:
            ShowRegisterWindow(hInst, hwnd); // <-- PASA hwnd como segundo parámetro
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        break;

    case WM_DESTROY:
        // PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
