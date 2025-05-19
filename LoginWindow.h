#pragma once

#include "byte_fix.h" 

#include <windows.h>

// Declaraciones externas de las variables globales
extern HINSTANCE hInst;
extern HWND hEmail;
extern HWND hPassword;
extern bool emailHasPlaceholder;
extern bool passwordHasPlaceholder;

// Declaraciones de funciones
void HandlePlaceholder(HWND hEdit, bool& hasPlaceholder, const wchar_t* placeholderText);
void Login(HWND hwnd);
void ShowLoginWindow(HINSTANCE hInstance);