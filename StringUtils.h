#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>

// Convierte std::wstring a std::string (UTF-8)
std::string WStringToString(const std::wstring& wstr);

// Convierte std::string (UTF-8) a std::wstring
std::wstring StringToWString(const std::string& str);

// Normaliza ISBN: elimina caracteres no numéricos de un std::wstring
std::string NormalizeISBN(const std::wstring& isbnW);

// Convierte std::string (UTF-8) a std::wstring, útil para mensajes de PostgreSQL
std::wstring Utf8ToWstring(const std::string& utf8Str);

#endif // STRINGUTILS_H
