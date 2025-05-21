#pragma once

#include "byte_fix.h" 

#include "UserAuth.h"
#include <string>
#include <libpq-fe.h> // Para PGconn

using namespace std;

extern HINSTANCE hInst;
extern HWND hEmail;
extern HWND hPassword;
extern HWND hUsername;
extern HWND hRoleCombo;

extern bool emailHasPlaceholder;
extern bool passwordHasPlaceholder;

extern UserAuth* auth;
extern PGconn* conn;

extern wstring currentUser;
extern wstring gUserRole;
extern wstring currentRole;