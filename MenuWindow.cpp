#include <string>
#include "MenuWindow.h"
#include "LoginWindow.h"
#include "AddBookWindow.h"
#include "LoanBookWindow.h"
#include "BuscarLibro.h"
#include "resources.h"

LRESULT CALLBACK MenuWndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE gInst;
std::wstring gUsername;

void ShowMenuWindow(HINSTANCE hInstance, const std::wstring& username)
{
    gInst = hInstance;
    gUsername = username;

    WNDCLASSW wc = {};
    wc.lpfnWndProc = MenuWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MenuWindow";
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    RegisterClassW(&wc);

    std::wstring windowTitle = L"Menú Principal - Usuario: " + username;

    HWND hwnd = CreateWindowW(L"MenuWindow", windowTitle.c_str(), WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, NULL, NULL, hInstance, NULL);

    SendMessage(hwnd, WM_SETICON, ICON_BIG, 
        (LPARAM)LoadIcon(gInst, MAKEINTRESOURCE(IDI_ICON1)));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL,
        (LPARAM)LoadImage(gInst, MAKEINTRESOURCE(IDI_ICON1), 
        IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));


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
        CreateWindowW(L"BUTTON", L"Agregar Libro", WS_VISIBLE | WS_CHILD, 100, 30, 200, 40, hwnd, (HMENU)1, gInst, NULL);
        CreateWindowW(L"BUTTON", L"Buscar Libro", WS_VISIBLE | WS_CHILD, 100, 80, 200, 40, hwnd, (HMENU)2, gInst, NULL);
        CreateWindowW(L"BUTTON", L"Listar Libros", WS_VISIBLE | WS_CHILD, 100, 130, 200, 40, hwnd, (HMENU)3, gInst, NULL);
        CreateWindowW(L"BUTTON", L"Prestar Libros", WS_VISIBLE | WS_CHILD, 100, 180, 200, 40, hwnd, (HMENU)4, gInst, NULL);
        CreateWindowW(L"BUTTON", L"Devolver Libros", WS_VISIBLE | WS_CHILD, 100, 230, 200, 40, hwnd, (HMENU)5, gInst, NULL);
        CreateWindowW(L"BUTTON", L"Cerrar Sesión", WS_VISIBLE | WS_CHILD, 100, 300, 180, 20, hwnd, (HMENU)6, NULL, NULL);
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
            ShowBuscarLibroWindow(gInst, gUsername);
            break;
        case 3:
            MessageBoxW(hwnd, L"Aquí se mostraría la lista de libros.", L"Listar", MB_OK);
            break;
        case 4:
        ShowWindow(hwnd, SW_HIDE);                // 👈 solo ocultamos el menú
    ShowLoanBookWindow(gInst, gUsername, hwnd); 
            break;
        case 5:
            MessageBoxW(hwnd, L"Aquí se abriría la ventana de Devolución.", L"Devolver", MB_OK);
            break;
        case 6: // ✅ Cerrar Sesión
            DestroyWindow(hwnd);       // Cierra la ventana de menú
            ShowLoginWindow(gInst);    // Vuelve a mostrar el login
            return 0;
        }
        break;

    case WM_DESTROY: 
      //PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
