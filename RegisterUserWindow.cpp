#include "byte_fix.h"
#include "RegisterUserWindow.h"
#include "GlobalVars.h"
#include "MenuWindow.h"
#include "WindowUtils.h"
#include "resources.h"
#include "EmailSender.h"
#include <iostream> // para wcerr
#include <ostream>
#include <string>
#include <libpq-fe.h>

extern PGconn *conn;
HWND hCorreo = nullptr;
HWND hMenuWindow = nullptr;

void RegisterNewUser(HWND hwnd)
{
    wchar_t username[100], correo[100], password[100], rol[50];
    GetWindowTextW(hUsername, username, 100);
    GetWindowTextW(hCorreo, correo, 100);
    GetWindowTextW(hPassword, password, 100);

    int sel = (int)SendMessage(hRoleCombo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR)
        sel = 0;
    const wchar_t *roles[] = {L"Lector", L"Bibliotecario"};
    wcscpy_s(rol, 50, roles[sel]);

    if (wcslen(username) == 0 || wcslen(correo) == 0)
    {
        MessageBoxW(hwnd, L"Todos los campos son obligatorios", L"Error", MB_ICONERROR);
        return;
    }

    wstring wUsername(username);
    wstring wCorreo(correo);
    wstring wPassword(password);
    wstring wRol(rol);

    string sUsername(wUsername.begin(), wUsername.end());
    string sCorreo(wCorreo.begin(), wCorreo.end());
    string sPassword(wPassword.begin(), wPassword.end());
    string sRol(wRol.begin(), wRol.end());

    // Puedes usar el mismo hash que tu clase UserAuth
    hash<string> hasher;
    string hashed = to_string(hasher(sPassword + "somesalt"));

    const char *paramValues[4] = {
        sUsername.c_str(),
        hashed.c_str(),
        sCorreo.c_str(),
        sRol.c_str()};

    const char *query = "INSERT INTO usuarios (username, password, email, rol) VALUES ($1, $2, $3, $4)";
    PGresult *res = PQexecParams(conn, query, 4, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) == PGRES_COMMAND_OK)
    {
        try
        {
            wstring wUsername(username);
            string sUsername(wUsername.begin(), wUsername.end());

            EmailSender::sendWelcomeEmail(sCorreo, sUsername);

            // Mostrar mensaje de confirmación
            MessageBoxW(hwnd, L"Usuario registrado exitosamente.\nSe ha enviado un correo de bienvenida.", L"Éxito", MB_ICONINFORMATION);
        }
        catch (const exception &e)
        {
            wcerr << L"[ERROR] No se pudo enviar el correo de bienvenida: " << e.what() << endl;

            // Mostrar mensaje aunque haya fallado el correo
            MessageBoxW(hwnd, L"Usuario registrado, pero no se pudo enviar el correo de bienvenida.", L"Advertencia", MB_ICONWARNING);
        }

        DestroyWindow(hwnd);
        ShowWindow(hMenuWindow, SW_SHOW);
    }
    else
    {
        MessageBoxW(hwnd, L"Error al registrar el usuario", L"Error", MB_ICONERROR);
    }

    PQclear(res);
}

LRESULT CALLBACK RegWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        // --- TU CÓDIGO AQUÍ ---
        CreateWindowW(L"STATIC", L"Usuario:", WS_VISIBLE | WS_CHILD, 20, 20, 80, 20, hwnd, NULL, NULL, NULL);
        hUsername = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 20, 250, 20, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"STATIC", L"Correo:", WS_VISIBLE | WS_CHILD, 20, 60, 80, 20, hwnd, NULL, NULL, NULL);
        hCorreo = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 60, 250, 20, hwnd, NULL, NULL, NULL);

        //CreateWindowW(L"STATIC", L"Contraseña:", WS_VISIBLE | WS_CHILD, 20, 100, 80, 20, hwnd, NULL, NULL, NULL);
        //hPassword = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 110, 100, 250, 20, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"STATIC", L"Rol:", WS_VISIBLE | WS_CHILD, 20, 140, 80, 20, hwnd, NULL, NULL, NULL);
        hRoleCombo = CreateWindowW(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_BORDER, 110, 140, 250, 100, hwnd, NULL, NULL, NULL);
        SendMessage(hRoleCombo, CB_ADDSTRING, 0, (LPARAM)L"Lector");
        SendMessage(hRoleCombo, CB_ADDSTRING, 0, (LPARAM)L"Bibliotecario");
        SendMessage(hRoleCombo, CB_SETCURSEL, 0, 0);

        // Centramos los botones
        RECT rect;
        GetClientRect(hwnd, &rect);

        int totalWidth = rect.right - rect.left;
        int buttonWidth = 100;
        int buttonHeight = 30;
        int spacing = 20;
        int totalButtonsWidth = buttonWidth * 2 + spacing;
        int startX = (totalWidth - totalButtonsWidth) / 2;
        int y = 190;

        CreateWindowW(L"BUTTON", L"Registrar", WS_VISIBLE | WS_CHILD, startX, y, buttonWidth, buttonHeight, hwnd, (HMENU)1, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Regresar", WS_VISIBLE | WS_CHILD, startX + buttonWidth + spacing, y, buttonWidth, buttonHeight, hwnd, (HMENU)2, NULL, NULL);

        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1)
        {
            RegisterNewUser(hwnd);
        }
        if (LOWORD(wParam) == 2)
        {
            DestroyWindow(hwnd);
            ShowWindow(hMenuWindow, SW_SHOW);
        }

        break;

    case WM_DESTROY:
        DestroyWindow(hwnd);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void ShowRegisterWindow(HINSTANCE hInstance, HWND menuWindow)
{

    hMenuWindow = menuWindow;        // <- guardar referencia del menú
    ShowWindow(menuWindow, SW_HIDE); // ocultar menú

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = RegWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"RegisterWindow";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"RegisterWindow", L"Registrar Nuevo Usuario",
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              CW_USEDEFAULT, CW_USEDEFAULT, 500, 300,
                              NULL, NULL, hInstance, NULL);

    WindowUtils::CenterWindow(hwnd);
}
