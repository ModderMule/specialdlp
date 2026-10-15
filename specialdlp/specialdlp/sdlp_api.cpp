// C ABI of specialdlp, see include/specialdlp.h. Converts UTF-16 <-> wchar_t at the
// boundary and forwards to the upstream rule functions in antiLeech.cpp.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later version.

// std headers first: the compat macros (_wcsicmp, ...) must not touch CRT declarations
#include <atomic>
#include <climits>
#include <cwchar>
#include <mutex>
#include <string>
#include <unordered_map>

#include "specialdlp.h"
#include "sdlp_rules.h"

#ifndef SDLP_LIB_VERSION
#define SDLP_LIB_VERSION "0.0.0"
#endif

namespace {

std::atomic<unsigned> g_variant{static_cast<unsigned>(SdlpVariant::Lite)};

constexpr bool kWide16 = WCHAR_MAX <= 0xFFFF;

// UTF-16 -> wchar_t (UTF-32 on POSIX). Unpaired surrogates pass through as-is.
std::wstring toWide(const sdlp_char16* s)
{
    std::wstring out;
    if (s == nullptr)
        return out;
    for (; *s != 0; ++s) {
        const std::uint32_t c = *s;
        if (!kWide16 && c >= 0xD800 && c <= 0xDBFF && s[1] >= 0xDC00 && s[1] <= 0xDFFF) {
            const std::uint32_t lo = *++s;
            out.push_back(static_cast<wchar_t>(0x10000 + ((c - 0xD800) << 10) + (lo - 0xDC00)));
        } else {
            out.push_back(static_cast<wchar_t>(c));
        }
    }
    return out;
}

// Reason strings are static literals; convert each once and keep it for the lib lifetime.
const sdlp_char16* toUtf16Static(const wchar_t* reason)
{
    if (reason == nullptr)
        return nullptr;
    if constexpr (kWide16) {
        return reinterpret_cast<const sdlp_char16*>(reason);
    } else {
        static std::mutex mutex;
        static std::unordered_map<const wchar_t*, std::u16string> cache;
        std::lock_guard lock(mutex);
        auto it = cache.find(reason);
        if (it == cache.end()) {
            std::u16string u;
            for (const wchar_t* p = reason; *p != L'\0'; ++p) {
                const auto c = static_cast<std::uint32_t>(*p);
                if (c >= 0x10000) {
                    u.push_back(static_cast<char16_t>(0xD800 + ((c - 0x10000) >> 10)));
                    u.push_back(static_cast<char16_t>(0xDC00 + ((c - 0x10000) & 0x3FF)));
                } else {
                    u.push_back(static_cast<char16_t>(c));
                }
            }
            it = cache.emplace(reason, std::move(u)).first;
        }
        return it->second.c_str();
    }
}

// NULL stays NULL for the LPCTSTR rules (they treat NULL as "clean")
template <typename Fn>
const sdlp_char16* check1(Fn fn, const sdlp_char16* a)
{
    if (a == nullptr)
        return nullptr;
    const std::wstring wa = toWide(a);
    return toUtf16Static(fn(wa.c_str()));
}

template <typename Fn>
const sdlp_char16* check2(Fn fn, const sdlp_char16* a, const sdlp_char16* b)
{
    if (a == nullptr || b == nullptr)
        return nullptr;
    const std::wstring wa = toWide(a);
    const std::wstring wb = toWide(b);
    return toUtf16Static(fn(wa.c_str(), wb.c_str()));
}

} // namespace

SdlpVariant sdlpVariant() noexcept
{
    return static_cast<SdlpVariant>(g_variant.load(std::memory_order_relaxed));
}

void sdlpSetVariant(SdlpVariant v) noexcept
{
    g_variant.store(static_cast<unsigned>(v), std::memory_order_relaxed);
}

extern "C" {

SDLP_API uint32_t sdlp_abi_version(void)
{
    return SDLP_ABI_VERSION;
}

SDLP_API uint32_t sdlp_rules_version(void)
{
    return GetDLPVersion();
}

SDLP_API const char* sdlp_lib_version(void)
{
    return SDLP_LIB_VERSION;
}

SDLP_API int32_t sdlp_set_variant(uint32_t variant)
{
    if (variant > SDLP_VARIANT_VERYCD_TAG)
        return -1;
    sdlpSetVariant(static_cast<SdlpVariant>(variant));
    return 0;
}

SDLP_API uint32_t sdlp_get_variant(void)
{
    return static_cast<uint32_t>(sdlpVariant());
}

SDLP_API const sdlp_char16* sdlp_check_modstring_hard(const sdlp_char16* modversion, const sdlp_char16* clientversion)
{
    return check2(DLPCheckModstring_Hard, modversion, clientversion);
}

SDLP_API const sdlp_char16* sdlp_check_modstring_soft(const sdlp_char16* modversion, const sdlp_char16* clientversion)
{
    return check2(DLPCheckModstring_Soft, modversion, clientversion);
}

SDLP_API const sdlp_char16* sdlp_check_username_hard(const sdlp_char16* username)
{
    return check1(DLPCheckUsername_Hard, username);
}

SDLP_API const sdlp_char16* sdlp_check_username_soft(const sdlp_char16* username)
{
    return check1(DLPCheckUsername_Soft, username);
}

SDLP_API const sdlp_char16* sdlp_check_name_hash_mod(const sdlp_char16* username, const sdlp_char16* userhash_hex,
                                                     const sdlp_char16* modversion)
{
    // upstream takes CStrings; NULL behaves like an empty string there
    const CString name(toWide(username));
    const CString hash(toWide(userhash_hex));
    const CString mod(toWide(modversion));
    return toUtf16Static(DLPCheckNameAndHashAndMod(name, hash, mod));
}

SDLP_API const sdlp_char16* sdlp_check_message_spam(const sdlp_char16* message)
{
    return check1(DLPCheckMessageSpam, message);
}

SDLP_API const sdlp_char16* sdlp_check_userhash(const uint8_t* userhash)
{
    if (userhash == nullptr)
        return nullptr;
    return toUtf16Static(DLPCheckUserhash(const_cast<PBYTE>(userhash)));
}

SDLP_API const sdlp_char16* sdlp_check_hello_tag(uint32_t tag)
{
    return toUtf16Static(DLPCheckHelloTag(tag));
}

SDLP_API const sdlp_char16* sdlp_check_info_tag(uint32_t tag)
{
    return toUtf16Static(DLPCheckInfoTag(tag));
}

} // extern "C"
