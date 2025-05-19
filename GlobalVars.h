#pragma once

#include "byte_fix.h" 

#include "UserAuth.h"
#include <string>
#include <libpq-fe.h> // Para PGconn

using namespace std;

extern wstring currentUser; // Declaración

#pragma once

extern HINSTANCE hInst;
extern HWND hEmail;
extern HWND hPassword;
extern UserAuth *auth;
extern PGconn *conn;

extern bool emailHasPlaceholder;
extern bool passwordHasPlaceholder;