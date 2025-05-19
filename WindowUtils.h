
#pragma once
#include "byte_fix.h" 

class WindowUtils {
public:
    static void CenterWindow(HWND hwnd) {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
        int windowWidth = rc.right - rc.left;
        int windowHeight = rc.bottom - rc.top;
        
        SetWindowPos(hwnd, NULL, 
                    (screenWidth - windowWidth) / 2,
                    (screenHeight - windowHeight) / 2,
                    0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }
};