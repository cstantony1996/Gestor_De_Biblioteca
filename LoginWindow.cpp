#include "byte_fix.h" 
#include "UserAuth.h"
#include "Connection.h"
#include "MenuWindow.h"
#include "GlobalVars.h"
#include "WindowUtils.h"
#include "resources.h"
#include "Credenciales.h"
#include <curl/curl.h>

using namespace std;

void ShowLoginWindow(HINSTANCE hInstance);

void CreateConsole()
{
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);  // Redirige stdout a consola
    freopen_s(&fp, "CONOUT$", "w", stderr);  // Redirige stderr a consola
    freopen_s(&fp, "CONIN$", "r", stdin);    // Redirige stdin a consola

    // Opcional: establece título para la consola
    SetConsoleTitleW(L"Consola de Debug - Biblioteca");
}


void HandlePlaceholder(HWND hEdit, bool &hasPlaceholder, const wchar_t *placeholderText)
{
    if (hEdit == hPassword)
    {
        // Manejo especial para el campo de contraseña
        if (hasPlaceholder && GetFocus() == hPassword)
        {
            // Transición de placeholder a campo vacío
            SetWindowTextW(hPassword, L"");
            hasPlaceholder = false;
            // Activar caracteres de contraseña
            SendMessage(hPassword, EM_SETPASSWORDCHAR, (WPARAM)L'*', 0);
            // Forzar redibujado
            InvalidateRect(hPassword, NULL, TRUE);
        }
        else if (!hasPlaceholder && GetWindowTextLength(hPassword) == 0 && GetFocus() != hPassword)
        {
            // Volver a mostrar placeholder
            SendMessage(hPassword, EM_SETPASSWORDCHAR, 0, 0);
            SetWindowTextW(hPassword, placeholderText);
            hasPlaceholder = true;
            InvalidateRect(hPassword, NULL, TRUE);
        }
    }
    else
    {
        // Manejo normal para otros campos (como el email)
        wchar_t currentText[100];
        GetWindowTextW(hEdit, currentText, 100);

        if (hasPlaceholder && (GetFocus() == hEdit || wcscmp(currentText, placeholderText) != 0))
        {
            if (wcscmp(currentText, placeholderText) == 0)
            {
                SetWindowTextW(hEdit, L"");
            }
            hasPlaceholder = false;
        }
        else if (!hasPlaceholder && GetWindowTextLength(hEdit) == 0 && GetFocus() != hEdit)
        {
            SetWindowTextW(hEdit, placeholderText);
            hasPlaceholder = true;
        }
    }
}

// Función de login
void Login(HWND hwnd)
{
    wchar_t email[100], password[100];
    GetWindowTextW(hEmail, email, 100);
    GetWindowTextW(hPassword, password, 100);

    // Verificar si todavía están mostrando placeholders
    if (emailHasPlaceholder || passwordHasPlaceholder)
    {
        MessageBoxW(hwnd, L"Por favor ingresa tus datos", L"Advertencia", MB_ICONWARNING);
        return;
    }

    // Convertir a wstring
    wstring wemail(email);
    wstring wpassword(password);

    if (auth->loginByEmail(wemail, wpassword))
    {
        wstring role = auth->getUserRole(wemail);
        MessageBoxW(hwnd, L"¡Inicio exitoso!", L"Éxito", MB_OK);
        DestroyWindow(hwnd);
        ShowMenuWindow(hInst, wemail, role);
    }
    else
    {
        MessageBoxW(hwnd, L"Correo o contraseña incorrectos", L"Error", MB_ICONERROR);
    }
}

// Procedimiento de ventana principal
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        // Campo de email
        hEmail = CreateWindowW(L"EDIT", L"",
                               WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                               50, 30, 300, 24, hwnd, NULL, hInst, NULL);
        SetWindowTextW(hEmail, L"Correo electrónico");
        emailHasPlaceholder = true;

        // Campo de contraseña
        hPassword = CreateWindowW(L"EDIT", L"",
                                  WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                  50, 70, 300, 24, hwnd, NULL, hInst, NULL);
        SetWindowTextW(hPassword, L"Contraseña");
        SendMessage(hPassword, EM_SETPASSWORDCHAR, 0, 0);
        passwordHasPlaceholder = true;

        // Botón
        CreateWindowW(L"BUTTON", L"Iniciar Sesión",
                      WS_VISIBLE | WS_CHILD,
                      150, 120, 100, 30, hwnd, (HMENU)1, hInst, NULL);
        break;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == 1)
        {
            // Botón "Iniciar Sesión"
            Login(hwnd);
        }
        else
        {
            HWND hEdit = (HWND)lParam;

            // Detectar si el mensaje es de foco para campos de texto
            if ((hEdit == hEmail || hEdit == hPassword) &&
                (HIWORD(wParam) == EN_SETFOCUS || HIWORD(wParam) == EN_KILLFOCUS))
            {
                if (hEdit == hEmail)
                {
                    HandlePlaceholder(hEmail, emailHasPlaceholder, L"Correo electrónico");
                }
                else if (hEdit == hPassword)
                {
                    HandlePlaceholder(hPassword, passwordHasPlaceholder, L"Contraseña");
                }
            }
        }
        break;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        HWND hEdit = (HWND)lParam;

        if ((hEdit == hEmail && emailHasPlaceholder) ||
            (hEdit == hPassword && passwordHasPlaceholder))
        {
            SetTextColor(hdc, RGB(160, 160, 160));
            SetBkMode(hdc, TRANSPARENT);

            static HBRUSH hBrush = NULL;
            if (!hBrush)
            {
                hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            }
            return (LRESULT)hBrush;
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowLoginWindow(HINSTANCE hInstance)
{
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"LoginWindow";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(
        L"LoginWindow",
        L"Login - Biblioteca",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 250,
        NULL, NULL, hInstance, NULL);

    if (!hwnd)
        return;

    // Configurar iconos
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

    WindowUtils::CenterWindow(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

// Punto de entrada de la aplicación
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{

    curl_global_init(CURL_GLOBAL_DEFAULT);

    CreateConsole();  
    
    if (!Credenciales::cargarDesdeArchivo("C:/Users/casta/OneDrive/Escritorio/Gestor_De_Biblioteca/gmail_credentials.txt")) {
        MessageBoxA(nullptr, "No se pudieron cargar las credenciales de Gmail.", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    hInst = hInstance;
    conn = conectarDB();
    if (!conn)
        return 1;
    auth = new UserAuth(conn);

    ShowLoginWindow(hInst);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    PQfinish(conn);
    delete auth;

    curl_global_cleanup();

    return 0;
}