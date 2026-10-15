// Shared parser for tests/vectors.tsv (used by the vector test and the upstream diff tool).
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later version.

#ifndef SDLP_TEST_VECTORS_H
#define SDLP_TEST_VECTORS_H

#include <array>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sdlp::test {

inline constexpr std::array<std::string_view, 5> kVariantNames{"lite", "allverycd", "easymule", "nickname", "tag"};

struct Vector
{
    int line = 0;
    std::string func;
    std::vector<std::uint32_t> variants;        // SDLP_VARIANT_* ids
    std::array<std::u16string, 3> args;
    std::optional<std::u16string> expected;     // nullopt = clean
};

inline void appendCodePoint(std::u16string& out, std::uint32_t cp)
{
    if (cp >= 0x10000) {
        cp -= 0x10000;
        out.push_back(static_cast<char16_t>(0xD800 + (cp >> 10)));
        out.push_back(static_cast<char16_t>(0xDC00 + (cp & 0x3FF)));
    } else {
        out.push_back(static_cast<char16_t>(cp));
    }
}

// UTF-8 with \t \\ \xNN \uXXXX escapes -> UTF-16
inline std::u16string decodeField(std::string_view s)
{
    std::u16string out;
    for (std::size_t i = 0; i < s.size();) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c == '\\' && i + 1 < s.size()) {
            const char e = s[i + 1];
            if (e == 't') { out.push_back(u'\t'); i += 2; continue; }
            if (e == '\\') { out.push_back(u'\\'); i += 2; continue; }
            if (e == 'x' || e == 'u') {
                const std::size_t n = e == 'x' ? 2 : 4;
                out.push_back(static_cast<char16_t>(std::stoul(std::string(s.substr(i + 2, n)), nullptr, 16)));
                i += 2 + n;
                continue;
            }
        }
        std::uint32_t cp = c;
        std::size_t len = 1;
        if (c >= 0xF0) { cp = c & 0x07; len = 4; }
        else if (c >= 0xE0) { cp = c & 0x0F; len = 3; }
        else if (c >= 0xC0) { cp = c & 0x1F; len = 2; }
        for (std::size_t k = 1; k < len && i + k < s.size(); ++k)
            cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
        appendCodePoint(out, cp);
        i += len;
    }
    return out;
}

inline std::string toUtf8(std::u16string_view s)
{
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        std::uint32_t cp = s[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < s.size())
            cp = 0x10000 + ((cp - 0xD800) << 10) + (s[++i] - 0xDC00);
        if (cp < 0x20) {
            out += "\\x";
            out += "0123456789ABCDEF"[cp >> 4];
            out += "0123456789ABCDEF"[cp & 0xF];
        } else if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return out;
}

inline std::vector<std::string_view> splitTabs(std::string_view line)
{
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    for (;;) {
        const auto pos = line.find('\t', start);
        parts.push_back(line.substr(start, pos == std::string_view::npos ? std::string_view::npos : pos - start));
        if (pos == std::string_view::npos)
            break;
        start = pos + 1;
    }
    return parts;
}

// Returns false and sets error on malformed input.
inline bool loadVectors(const std::string& path, std::vector<Vector>& out, std::string& error)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "cannot open " + path;
        return false;
    }
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (lineNo == 1 && line.starts_with("\xEF\xBB\xBF"))
            line.erase(0, 3);
        if (line.empty() || line.front() == '#')
            continue;
        const auto parts = splitTabs(line);
        if (parts.size() != 6) {
            error = path + ":" + std::to_string(lineNo) + ": expected 6 fields, got " + std::to_string(parts.size());
            return false;
        }
        Vector v;
        v.line = lineNo;
        v.func = std::string(parts[0]);
        if (parts[1] == "*") {
            for (std::uint32_t i = 0; i < kVariantNames.size(); ++i)
                v.variants.push_back(i);
        } else {
            std::string_view rest = parts[1];
            while (!rest.empty()) {
                const auto comma = rest.find(',');
                const auto name = rest.substr(0, comma);
                bool found = false;
                for (std::uint32_t i = 0; i < kVariantNames.size(); ++i) {
                    if (kVariantNames[i] == name) {
                        v.variants.push_back(i);
                        found = true;
                    }
                }
                if (!found) {
                    error = path + ":" + std::to_string(lineNo) + ": unknown variant " + std::string(name);
                    return false;
                }
                rest = comma == std::string_view::npos ? std::string_view{} : rest.substr(comma + 1);
            }
        }
        for (std::size_t i = 0; i < 3; ++i)
            v.args[i] = decodeField(parts[2 + i]);
        if (parts[5] != "-")
            v.expected = decodeField(parts[5]);
        out.push_back(std::move(v));
    }
    return true;
}

inline std::uint32_t parseHex(const std::u16string& s)
{
    return static_cast<std::uint32_t>(std::stoul(toUtf8(s), nullptr, 16));
}

inline std::array<std::uint8_t, 16> parseHash(const std::u16string& s)
{
    std::array<std::uint8_t, 16> hash{};
    const std::string hex = toUtf8(s);
    for (std::size_t i = 0; i < 16 && i * 2 + 1 < hex.size(); ++i)
        hash[i] = static_cast<std::uint8_t>(std::stoul(hex.substr(i * 2, 2), nullptr, 16));
    return hash;
}

} // namespace sdlp::test

#endif // SDLP_TEST_VECTORS_H
