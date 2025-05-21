#include "byte_fix.h" 

#include "GlobalVars.h"

using namespace std;

HINSTANCE hInst = nullptr;
HWND hEmail = nullptr;
HWND hPassword = nullptr;
HWND hUsername = nullptr;
HWND hRoleCombo = nullptr;

bool emailHasPlaceholder = true;
bool passwordHasPlaceholder = true;

wstring currentUser = L"";
wstring gUserRole = L"";
wstring currentRole = L"";

UserAuth* auth = nullptr;
PGconn* conn = nullptr;
