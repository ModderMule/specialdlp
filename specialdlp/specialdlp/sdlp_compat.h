// Portable stand-ins for the ATL/shlwapi/TCHAR bits antiLeech.cpp uses, so the
// rule bodies stay byte-identical to upstream where possible (easy merges).
//
// Everything works on wchar_t (UTF-16 on Windows, UTF-32 elsewhere). The C API in
// sdlp_api.cpp converts from/to UTF-16 at the boundary.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later version.

#ifndef SDLP_COMPAT_H
#define SDLP_COMPAT_H

#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <string>

namespace sdlp::compat {

// --- character classes (deterministic, locale independent; mirror Win32 C1_* sets) ---
bool isPunct(std::uint32_t ch) noexcept;
bool isSpace(std::uint32_t ch) noexcept;
bool isCntrl(std::uint32_t ch) noexcept;
bool isDigit(std::uint32_t ch) noexcept;
bool isXDigit(std::uint32_t ch) noexcept;

// Simple Unicode upper-case mapping (stand-in for CharUpperW used by shlwapi).
wchar_t foldUpper(wchar_t ch) noexcept;

// --- shlwapi / CRT string functions ---
const wchar_t* strStrI(const wchar_t* hay, const wchar_t* needle) noexcept;   // StrStrI
const wchar_t* strStr(const wchar_t* hay, const wchar_t* needle) noexcept;    // StrStr, _tcsstr
int strCmpI(const wchar_t* a, const wchar_t* b) noexcept;                     // StrCmpI (Unicode fold)
int wcsICmpAscii(const wchar_t* a, const wchar_t* b) noexcept;                // _wcsicmp in "C" locale
double toDouble(const wchar_t* s) noexcept;                                   // _wtof, '.' decimal point

// --- minimal ATL CString ---
class CString
{
public:
    CString() = default;
    CString(const wchar_t* s) : m_s(s ? s : L"") {}
    CString(std::wstring s) : m_s(std::move(s)) {}

    [[nodiscard]] bool IsEmpty() const noexcept { return m_s.empty(); }
    [[nodiscard]] int GetLength() const noexcept { return static_cast<int>(m_s.size()); }
    [[nodiscard]] const wchar_t* GetString() const noexcept { return m_s.c_str(); }
    operator const wchar_t*() const noexcept { return m_s.c_str(); }

    [[nodiscard]] wchar_t GetAt(int i) const noexcept;       // out of range -> 0 (ATL asserts)
    [[nodiscard]] CString Left(int count) const;
    [[nodiscard]] CString Right(int count) const;
    [[nodiscard]] CString Mid(int first) const;
    [[nodiscard]] CString Mid(int first, int count) const;
    [[nodiscard]] int Find(const wchar_t* sub, int start = 0) const noexcept;
    [[nodiscard]] int Find(wchar_t ch, int start = 0) const noexcept;
    [[nodiscard]] int ReverseFind(wchar_t ch) const noexcept;
    CString& Trim();

    friend bool operator==(const CString& a, const wchar_t* b) noexcept { return a.m_s == (b ? b : L""); }
    friend bool operator==(const CString& a, const CString& b) noexcept { return a.m_s == b.m_s; }
    friend CString operator+(wchar_t ch, const CString& s) { return CString(ch + s.m_s); }

private:
    std::wstring m_s;
};

} // namespace sdlp::compat

// --- Win32 names used by the upstream rule code ---
// Include every std header before this file: the macros below would rewrite CRT
// declarations such as MSVC's _wcsicmp.
using CString = sdlp::compat::CString;
using TCHAR = wchar_t;
using LPCTSTR = const wchar_t*;
using DWORD = std::uint32_t;
using UINT = unsigned int;
using BYTE = std::uint8_t;
using PBYTE = BYTE*;
using _TINT = std::wint_t;

#ifndef NULL
#define NULL nullptr
#endif

#define _T(x) L##x

#define StrStrI(h, n)   sdlp::compat::strStrI((h), (n))
#define StrStrIW(h, n)  sdlp::compat::strStrI((h), (n))
#define StrStr(h, n)    sdlp::compat::strStr((h), (n))
#define _tcsstr(h, n)   sdlp::compat::strStr((h), (n))
#define StrCmpI(a, b)   sdlp::compat::strCmpI((a), (b))
#define _tcsicmp(a, b)  sdlp::compat::wcsICmpAscii((a), (b))
#define _wcsicmp(a, b)  sdlp::compat::wcsICmpAscii((a), (b))
#define _tcslen(s)      std::wcslen(s)
#define _tstof(s)       sdlp::compat::toDouble(s)
#define _istpunct(c)    sdlp::compat::isPunct(static_cast<std::uint32_t>(c))
#define _istspace(c)    sdlp::compat::isSpace(static_cast<std::uint32_t>(c))
#define _istcntrl(c)    sdlp::compat::isCntrl(static_cast<std::uint32_t>(c))
#define _istdigit(c)    sdlp::compat::isDigit(static_cast<std::uint32_t>(c))
#define _istxdigit(c)   sdlp::compat::isXDigit(static_cast<std::uint32_t>(c))

// Rule functions are internal; only sdlp_api.cpp exports a C ABI.
#define SDLP_RULE

#endif // SDLP_COMPAT_H
