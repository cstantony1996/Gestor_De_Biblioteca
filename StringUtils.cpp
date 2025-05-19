#include "byte_fix.h" 
#include "StringUtils.h"

using namespace std;

// Convierte wstring a string UTF-8
string WStringToString(const wstring &wstr)
{
    if (wstr.empty())
        return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
                                   nullptr, 0, nullptr, nullptr);
    string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
                        &result[0], size, nullptr, nullptr);
    return result;
}

// Convierte string UTF-8 a wstring
wstring StringToWString(const string &str)
{
    if (str.empty())
        return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size);
    return result;
}

// Normaliza el ISBN quitando caracteres no numéricos
string NormalizeISBN(const wstring &isbnW)
{
    wstring cleanIsbn;
    for (wchar_t c : isbnW)
    {
        if (iswdigit(c))
        {
            cleanIsbn += c;
        }
    }
    return WStringToString(cleanIsbn);
}

// Convierte string UTF-8 a wstring (se usa con mensajes de PostgreSQL)
wstring Utf8ToWstring(const string &utf8Str)
{
    if (utf8Str.empty())
        return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    wstring result(size_needed - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &result[0], size_needed);
    return result;
}
