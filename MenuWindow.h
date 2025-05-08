#ifndef MENUWINDOW_H
#define MENUWINDOW_H

#include <windows.h>
#include <string>

void ShowMenuWindow(HINSTANCE hInstance, const std::wstring& username);
void ShowAddBookWindow(HINSTANCE hInstance, const std::wstring& username);

#endif
