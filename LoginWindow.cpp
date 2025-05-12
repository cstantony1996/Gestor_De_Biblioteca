#include <string>
#include <windows.h>
#include "UserAuth.h"
#include "Connection.h"
#include "MenuWindow.h"
#include "LoginWindow.h"
#include "resources.h"

// Variables globales
HINSTANCE hInst;
HINSTANCE hInstGlobal;
HWND hUsername, hPassword;
HWND hNewUser, hNewPass, hNewEmail;
UserAuth *auth;
PGconn *conn;

// Declaración
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RegistroWndProc(HWND, UINT, WPARAM, LPARAM);
void ShowLoginWindow(HINSTANCE hInstance);
void ShowRegisterWindow(HWND hwndLogin);
void login(HWND hwnd);

// WinMain con Unicode
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{

    hInstGlobal = hInstance;

    conn = conectarDB();
    if (!conn)
        return 1;
    auth = new UserAuth(conn);
    hInst = hInstance;

    // Registrar clase login
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"LoginWindow";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));  // Icono principal
    RegisterClassW(&wc);

    // Registrar clase registro
    WNDCLASSW wcReg = {};
    wcReg.lpfnWndProc = RegistroWndProc;
    wcReg.hInstance = hInstance;
    wcReg.lpszClassName = L"RegistroWindow";
    wcReg.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    RegisterClassW(&wcReg);

    ShowLoginWindow(hInstance);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    PQfinish(conn);
    delete auth;
    return 0;
}

// Mostrar ventana login
void ShowLoginWindow(HINSTANCE hInstance) {
    hInst = hInstance;

    HWND hwnd = CreateWindowW(L"LoginWindow", L"Login - Biblioteca", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 400, 250, NULL, NULL, hInst, NULL);

    SendMessage(hwnd, WM_SETICON, ICON_BIG, 
        (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL,
        (LPARAM)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), 
        IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

// Mostrar ventana de registro
void ShowRegisterWindow(HWND hwndLogin)
{
    ShowWindow(hwndLogin, SW_HIDE); // Solo ocultar
    HWND hwndRegistro = CreateWindowW(L"RegistroWindow", L"Registro de Usuario", WS_OVERLAPPEDWINDOW,
                                      CW_USEDEFAULT, CW_USEDEFAULT, 450, 300, NULL, NULL, hInst, hwndLogin);
    ShowWindow(hwndRegistro, SW_SHOW);
    UpdateWindow(hwndRegistro);
}

// Función de login
void login(HWND hwnd)
{
    wchar_t username[100], password[100];
    GetWindowTextW(hUsername, username, 100);
    GetWindowTextW(hPassword, password, 100);

    char u[100], p[100];
    wcstombs(u, username, 100);
    wcstombs(p, password, 100);

    if (auth->login(u, p))
    {
        MessageBoxW(hwnd, L"¡Inicio de sesión exitoso!", L"Éxito", MB_OK);
        DestroyWindow(hwnd); // Cierra la ventana de login
        ShowMenuWindow(hInstGlobal, username);
    }
    else
    {
        MessageBoxW(hwnd, L"Usuario o contraseña incorrectos.", L"Error", MB_OK | MB_ICONERROR);
    }
}

// Ventana Login
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"Usuario:", WS_VISIBLE | WS_CHILD, 50, 30, 80, 20, hwnd, NULL, NULL, NULL);
        hUsername = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                  130, 30, 200, 24, hwnd, NULL, hInst, NULL);

        CreateWindowW(L"STATIC", L"Contraseña:", WS_VISIBLE | WS_CHILD, 50, 70, 80, 20, hwnd, NULL, NULL, NULL);
        hPassword = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL,
                                  130, 70, 200, 24, hwnd, NULL, hInst, NULL);

        CreateWindowW(L"BUTTON", L"Iniciar Sesión", WS_VISIBLE | WS_CHILD, 60, 130, 110, 30, hwnd, (HMENU)1, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Registrarse", WS_VISIBLE | WS_CHILD, 200, 130, 110, 30, hwnd, (HMENU)2, NULL, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1:
            login(hwnd);
            break;
        case 2:
            ShowRegisterWindow(hwnd);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Ventana Registro
LRESULT CALLBACK RegistroWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndLogin;

    switch (msg)
    {
    case WM_CREATE:
        hwndLogin = (HWND)((LPCREATESTRUCT)lParam)->lpCreateParams;

        CreateWindowW(L"STATIC", L"Nuevo Usuario:", WS_VISIBLE | WS_CHILD, 30, 30, 120, 20, hwnd, NULL, NULL, NULL);
        hNewUser = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                 160, 30, 220, 24, hwnd, NULL, hInst, NULL);

        CreateWindowW(L"STATIC", L"Nueva Contraseña:", WS_VISIBLE | WS_CHILD, 30, 70, 150, 20, hwnd, NULL, NULL, NULL);
        hNewPass = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL,
                                 160, 70, 220, 24, hwnd, NULL, hInst, NULL);

        CreateWindowW(L"STATIC", L"Correo:", WS_VISIBLE | WS_CHILD, 30, 110, 120, 20, hwnd, NULL, NULL, NULL);
        hNewEmail = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                  160, 110, 220, 24, hwnd, NULL, hInst, NULL);

        CreateWindowW(L"BUTTON", L"Aceptar", WS_VISIBLE | WS_CHILD, 160, 160, 120, 30, hwnd, (HMENU)3, NULL, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 3)
        {
            wchar_t username[100], password[100], email[100];
            GetWindowTextW(hNewUser, username, 100);
            GetWindowTextW(hNewPass, password, 100);
            GetWindowTextW(hNewEmail, email, 100);

            // Convertir a std::string
            char u[100], p[100], e[100];
            wcstombs(u, username, 100);
            wcstombs(p, password, 100);
            wcstombs(e, email, 100);

            if (auth->registerUser(u, p, e))
            {
                MessageBoxW(hwnd, L"Registro exitoso. Puede iniciar sesión.", L"Éxito", MB_OK);
                DestroyWindow(hwnd);
                ShowWindow(hwndLogin, SW_SHOW); // Volver a mostrar login
            }
            else
            {
                MessageBoxW(hwnd, L"Error al registrar. ¿Usuario ya existe?", L"Error", MB_OK | MB_ICONERROR);
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
