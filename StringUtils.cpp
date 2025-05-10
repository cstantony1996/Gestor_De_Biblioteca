#include "StringUtils.h"

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
                                 nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
                            &result[0], size, nullptr, nullptr);
    return result;
}

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size);
    return result;
}

std::string NormalizeISBN(const std::wstring& isbnW) {
    std::wstring cleanIsbn;
    for (wchar_t c : isbnW) {
        if (iswdigit(c)) {
            cleanIsbn += c;
        }
    }
    return WStringToString(cleanIsbn);
}