#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include "byte_fix.h" 
#include <string>

using namespace std;

// Convierte wstring a string (UTF-8)
string WStringToString(const wstring& wstr);

// Convierte string (UTF-8) a wstring
wstring StringToWString(const string& str);

// Normaliza ISBN: elimina caracteres no numéricos de un wstring
string NormalizeISBN(const wstring& isbnW);

// Convierte string (UTF-8) a wstring, útil para mensajes de PostgreSQL
wstring Utf8ToWstring(const string& utf8Str);

#endif // STRINGUTILS_H
