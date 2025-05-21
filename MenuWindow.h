#ifndef MENUWINDOW_H
#define MENUWINDOW_H

#include "byte_fix.h" 
#include <string>

using namespace std;

void ShowMenuWindow(HINSTANCE hInstance, const wstring& username, const wstring& userRole);
void ShowAddBookWindow(HINSTANCE hInstance, const wstring& username);

#endif
