// Portable stand-ins for ATL/shlwapi/TCHAR, see sdlp_compat.h.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later version.

// std headers first: the compat macros (_wcsicmp, ...) must not touch CRT declarations
#include <algorithm>
#include <charconv>

#include "sdlp_compat.h"

namespace sdlp::compat {

namespace {

constexpr bool inRange(std::uint32_t ch, std::uint32_t lo, std::uint32_t hi) noexcept
{
    return ch >= lo && ch <= hi;
}

} // namespace

bool isCntrl(std::uint32_t ch) noexcept
{
    return ch < 0x20 || inRange(ch, 0x7F, 0x9F);
}

bool isSpace(std::uint32_t ch) noexcept
{
    // Unicode White_Space (what Win32 C1_SPACE reports for the BMP)
    return inRange(ch, 0x09, 0x0D) || ch == 0x20 || ch == 0x85 || ch == 0xA0 || ch == 0x1680
        || inRange(ch, 0x2000, 0x200A) || ch == 0x2028 || ch == 0x2029 || ch == 0x202F
        || ch == 0x205F || ch == 0x3000;
}

bool isDigit(std::uint32_t ch) noexcept
{
    return inRange(ch, '0', '9');
}

bool isXDigit(std::uint32_t ch) noexcept
{
    return inRange(ch, '0', '9') || inRange(ch, 'A', 'F') || inRange(ch, 'a', 'f');
}

bool isPunct(std::uint32_t ch) noexcept
{
    // ASCII
    if (inRange(ch, 0x21, 0x2F) || inRange(ch, 0x3A, 0x40) || inRange(ch, 0x5B, 0x60) || inRange(ch, 0x7B, 0x7E))
        return true;
    // Latin-1: ¡..¿ (incl. «»´§), × and ÷
    if (inRange(ch, 0xA1, 0xBF) || ch == 0xD7 || ch == 0xF7)
        return true;
    // General Punctuation, currency, letterlike arrows/math are mostly symbols in Win32 too
    if (inRange(ch, 0x2010, 0x2027) || inRange(ch, 0x2030, 0x205E) || inRange(ch, 0x20A0, 0x20CF))
        return true;
    // CJK symbols and punctuation
    if (inRange(ch, 0x3001, 0x3003) || inRange(ch, 0x3008, 0x3011) || inRange(ch, 0x3014, 0x301F))
        return true;
    // Fullwidth ASCII punctuation
    if (inRange(ch, 0xFF01, 0xFF0F) || inRange(ch, 0xFF1A, 0xFF20) || inRange(ch, 0xFF3B, 0xFF40)
        || inRange(ch, 0xFF5B, 0xFF65))
        return true;
    return false;
}

wchar_t foldUpper(wchar_t wc) noexcept
{
    const auto ch = static_cast<std::uint32_t>(wc);
    auto out = ch;
    if (inRange(ch, 'a', 'z'))
        out = ch - 0x20;
    else if (ch < 0xB5)
        out = ch;
    else if (ch == 0xB5)
        out = 0x39C;                                    // micro sign -> Greek Mu
    else if (inRange(ch, 0xE0, 0xFE) && ch != 0xF7)
        out = ch - 0x20;
    else if (ch == 0xFF)
        out = 0x178;
    else if (inRange(ch, 0x100, 0x137) || inRange(ch, 0x14A, 0x177))
        out = ch & ~1u;                                 // pairs: even upper, odd lower
    else if (inRange(ch, 0x139, 0x148) || inRange(ch, 0x179, 0x17E))
        out = (ch & 1u) ? ch : ch - 1;                  // pairs: odd upper, even lower
    else if (inRange(ch, 0x3B1, 0x3C9) && ch != 0x3C2)
        out = ch - 0x20;
    else if (ch == 0x3C2)
        out = 0x3A3;                                    // final sigma
    else if (inRange(ch, 0x430, 0x44F))
        out = ch - 0x20;
    else if (inRange(ch, 0x450, 0x45F))
        out = ch - 0x50;
    else if (inRange(ch, 0xFF41, 0xFF5A))
        out = ch - 0x20;                                // fullwidth a-z
    return static_cast<wchar_t>(out);
}

const wchar_t* strStrI(const wchar_t* hay, const wchar_t* needle) noexcept
{
    if (hay == nullptr || needle == nullptr || *needle == L'\0')
        return nullptr;
    const std::size_t n = std::wcslen(needle);
    for (const wchar_t* p = hay; *p != L'\0'; ++p) {
        std::size_t i = 0;
        while (i < n && p[i] != L'\0' && foldUpper(p[i]) == foldUpper(needle[i]))
            ++i;
        if (i == n)
            return p;
    }
    return nullptr;
}

const wchar_t* strStr(const wchar_t* hay, const wchar_t* needle) noexcept
{
    if (hay == nullptr || needle == nullptr || *needle == L'\0')
        return nullptr;
    return std::wcsstr(hay, needle);
}

int strCmpI(const wchar_t* a, const wchar_t* b) noexcept
{
    if (a == nullptr || b == nullptr)
        return a == b ? 0 : (a == nullptr ? -1 : 1);
    for (;; ++a, ++b) {
        const auto ca = static_cast<std::uint32_t>(foldUpper(*a));
        const auto cb = static_cast<std::uint32_t>(foldUpper(*b));
        if (ca != cb)
            return ca < cb ? -1 : 1;
        if (ca == 0)
            return 0;
    }
}

int wcsICmpAscii(const wchar_t* a, const wchar_t* b) noexcept
{
    const auto lower = [](wchar_t c) -> std::uint32_t {
        const auto u = static_cast<std::uint32_t>(c);
        return inRange(u, 'A', 'Z') ? u + 0x20 : u;
    };
    if (a == nullptr || b == nullptr)
        return a == b ? 0 : (a == nullptr ? -1 : 1);
    for (;; ++a, ++b) {
        const auto ca = lower(*a);
        const auto cb = lower(*b);
        if (ca != cb)
            return ca < cb ? -1 : 1;
        if (ca == 0)
            return 0;
    }
}

double toDouble(const wchar_t* s) noexcept
{
    if (s == nullptr)
        return 0.0;
    while (*s != L'\0' && isSpace(static_cast<std::uint32_t>(*s)))
        ++s;

    // narrow the ASCII number prefix; anything else ends the number like _wtof
    std::string buf;
    for (const wchar_t* p = s; *p != L'\0' && static_cast<std::uint32_t>(*p) < 0x80 && buf.size() < 64; ++p)
        buf.push_back(static_cast<char>(*p));

    std::size_t pos = 0;
    bool negative = false;
    if (pos < buf.size() && (buf[pos] == '+' || buf[pos] == '-'))
        negative = buf[pos++] == '-';

    auto fmt = std::chars_format::general;
    if (buf.size() - pos > 2 && buf[pos] == '0' && (buf[pos + 1] == 'x' || buf[pos + 1] == 'X')) {
        fmt = std::chars_format::hex;
        pos += 2;
    }

    double value = 0.0;
    const auto res = std::from_chars(buf.data() + pos, buf.data() + buf.size(), value, fmt);
    if (res.ec != std::errc{})
        return 0.0;
    return negative ? -value : value;
}

// --- CString ---

wchar_t CString::GetAt(int i) const noexcept
{
    return (i >= 0 && i < GetLength()) ? m_s[static_cast<std::size_t>(i)] : L'\0';
}

CString CString::Left(int count) const
{
    count = std::clamp(count, 0, GetLength());
    return CString(m_s.substr(0, static_cast<std::size_t>(count)));
}

CString CString::Right(int count) const
{
    count = std::clamp(count, 0, GetLength());
    return CString(m_s.substr(static_cast<std::size_t>(GetLength() - count)));
}

CString CString::Mid(int first) const
{
    first = std::max(first, 0);
    if (first >= GetLength())
        return {};
    return CString(m_s.substr(static_cast<std::size_t>(first)));
}

CString CString::Mid(int first, int count) const
{
    first = std::max(first, 0);
    count = std::max(count, 0);
    if (first >= GetLength())
        return {};
    return CString(m_s.substr(static_cast<std::size_t>(first), static_cast<std::size_t>(count)));
}

int CString::Find(const wchar_t* sub, int start) const noexcept
{
    if (sub == nullptr || start < 0 || start > GetLength())
        return -1;
    const auto pos = m_s.find(sub, static_cast<std::size_t>(start));
    return pos == std::wstring::npos ? -1 : static_cast<int>(pos);
}

int CString::Find(wchar_t ch, int start) const noexcept
{
    if (start < 0 || start >= GetLength())
        return -1;
    const auto pos = m_s.find(ch, static_cast<std::size_t>(start));
    return pos == std::wstring::npos ? -1 : static_cast<int>(pos);
}

int CString::ReverseFind(wchar_t ch) const noexcept
{
    const auto pos = m_s.rfind(ch);
    return pos == std::wstring::npos ? -1 : static_cast<int>(pos);
}

CString& CString::Trim()
{
    const auto notSpace = [](wchar_t c) { return !isSpace(static_cast<std::uint32_t>(c)); };
    const auto first = std::find_if(m_s.begin(), m_s.end(), notSpace);
    const auto last = std::find_if(m_s.rbegin(), m_s.rend(), notSpace).base();
    m_s = (first < last) ? std::wstring(first, last) : std::wstring();
    return *this;
}

} // namespace sdlp::compat
