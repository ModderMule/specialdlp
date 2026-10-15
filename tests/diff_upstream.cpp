// Windows only: runs every vector through an upstream antiLeech*.dll (built from the
// original MFC sources) and through this library, and reports any difference.
// The "expected" column is ignored; this checks the port, not the vectors.
//
// usage: sdlp_diff_upstream <antiLeech.dll> <variant> <vectors.tsv>
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later version.

#include "specialdlp.h"
#include "vectors.h"

#include <atlstr.h>
#include <windows.h>

#include <cstdio>

namespace {

// Signatures as used by eMule Xtreme (DLP.h)
using GetDLPVersionFn = DWORD(__cdecl*)();
using CheckModFn = LPCTSTR(__cdecl*)(LPCTSTR modversion, LPCTSTR clientversion);
using CheckNameFn = LPCTSTR(__cdecl*)(LPCTSTR username);
using CheckNameHashModFn = LPCTSTR(__cdecl*)(CString username, CString& userhash, CString& modversion);
using CheckSpamFn = LPCTSTR(__cdecl*)(LPCTSTR messagetext);
using CheckUserhashFn = LPCTSTR(__cdecl*)(const PBYTE userhash);
using CheckTagFn = LPCTSTR(__cdecl*)(UINT tagnumber);

struct Upstream
{
    GetDLPVersionFn version = nullptr;
    CheckModFn modHard = nullptr;
    CheckModFn modSoft = nullptr;
    CheckNameFn userHard = nullptr;
    CheckNameFn userSoft = nullptr;
    CheckNameHashModFn nameHashMod = nullptr;
    CheckSpamFn spam = nullptr;
    CheckUserhashFn userhash = nullptr;
    CheckTagFn helloTag = nullptr;
    CheckTagFn infoTag = nullptr;
};

template <typename T>
bool resolve(HMODULE lib, const char* name, T& fn)
{
    fn = reinterpret_cast<T>(::GetProcAddress(lib, name));
    if (fn == nullptr)
        std::fprintf(stderr, "missing export %s\n", name);
    return fn != nullptr;
}

std::optional<std::u16string> wrapW(LPCTSTR r)
{
    return r ? std::optional<std::u16string>(reinterpret_cast<const char16_t*>(r)) : std::nullopt;
}

std::optional<std::u16string> wrap(const sdlp_char16* r)
{
    return r ? std::optional<std::u16string>(r) : std::nullopt;
}

LPCTSTR w(const std::u16string& s)
{
    return reinterpret_cast<LPCTSTR>(s.c_str());
}

std::optional<std::u16string> runUpstream(const Upstream& up, const sdlp::test::Vector& v)
{
    const auto& a = v.args;
    if (v.func == "modhard") return wrapW(up.modHard(w(a[0]), w(a[1])));
    if (v.func == "modsoft") return wrapW(up.modSoft(w(a[0]), w(a[1])));
    if (v.func == "userhard") return wrapW(up.userHard(w(a[0])));
    if (v.func == "usersoft") return wrapW(up.userSoft(w(a[0])));
    if (v.func == "namehashmod") {
        CString hash(w(a[1]));
        CString mod(w(a[2]));
        return wrapW(up.nameHashMod(CString(w(a[0])), hash, mod));
    }
    if (v.func == "spam") return wrapW(up.spam(w(a[0])));
    if (v.func == "hellotag") return wrapW(up.helloTag(sdlp::test::parseHex(a[0])));
    if (v.func == "infotag") return wrapW(up.infoTag(sdlp::test::parseHex(a[0])));
    if (v.func == "userhash") {
        auto hash = sdlp::test::parseHash(a[0]);
        return wrapW(up.userhash(hash.data()));
    }
    return std::nullopt;
}

std::optional<std::u16string> runPort(const sdlp::test::Vector& v)
{
    const auto& a = v.args;
    if (v.func == "modhard") return wrap(sdlp_check_modstring_hard(a[0].c_str(), a[1].c_str()));
    if (v.func == "modsoft") return wrap(sdlp_check_modstring_soft(a[0].c_str(), a[1].c_str()));
    if (v.func == "userhard") return wrap(sdlp_check_username_hard(a[0].c_str()));
    if (v.func == "usersoft") return wrap(sdlp_check_username_soft(a[0].c_str()));
    if (v.func == "namehashmod") return wrap(sdlp_check_name_hash_mod(a[0].c_str(), a[1].c_str(), a[2].c_str()));
    if (v.func == "spam") return wrap(sdlp_check_message_spam(a[0].c_str()));
    if (v.func == "hellotag") return wrap(sdlp_check_hello_tag(sdlp::test::parseHex(a[0])));
    if (v.func == "infotag") return wrap(sdlp_check_info_tag(sdlp::test::parseHex(a[0])));
    if (v.func == "userhash") {
        const auto hash = sdlp::test::parseHash(a[0]);
        return wrap(sdlp_check_userhash(hash.data()));
    }
    return std::nullopt;
}

std::string show(const std::optional<std::u16string>& s)
{
    return s ? "\"" + sdlp::test::toUtf8(*s) + "\"" : "NULL";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 4) {
        std::fprintf(stderr, "usage: %s antiLeech.dll variant vectors.tsv\n", argv[0]);
        return 2;
    }

    std::uint32_t variant = UINT32_MAX;
    for (std::uint32_t i = 0; i < sdlp::test::kVariantNames.size(); ++i)
        if (sdlp::test::kVariantNames[i] == argv[2])
            variant = i;
    if (variant == UINT32_MAX) {
        std::fprintf(stderr, "unknown variant %s\n", argv[2]);
        return 2;
    }

    HMODULE lib = ::LoadLibraryA(argv[1]);
    if (lib == nullptr) {
        std::fprintf(stderr, "cannot load %s (error %lu)\n", argv[1], ::GetLastError());
        return 2;
    }
    Upstream up;
    const bool ok = resolve(lib, "GetDLPVersion", up.version) && resolve(lib, "DLPCheckModstring_Hard", up.modHard)
        && resolve(lib, "DLPCheckModstring_Soft", up.modSoft) && resolve(lib, "DLPCheckUsername_Hard", up.userHard)
        && resolve(lib, "DLPCheckUsername_Soft", up.userSoft)
        && resolve(lib, "DLPCheckNameAndHashAndMod", up.nameHashMod)
        && resolve(lib, "DLPCheckMessageSpam", up.spam) && resolve(lib, "DLPCheckUserhash", up.userhash)
        && resolve(lib, "DLPCheckHelloTag", up.helloTag) && resolve(lib, "DLPCheckInfoTag", up.infoTag);
    if (!ok)
        return 2;
    if (up.version() != sdlp_rules_version()) {
        std::fprintf(stderr, "rule version differs: upstream %lu, port %u\n", up.version(), sdlp_rules_version());
        return 1;
    }

    std::vector<sdlp::test::Vector> vectors;
    std::string error;
    if (!sdlp::test::loadVectors(argv[3], vectors, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 2;
    }

    sdlp_set_variant(variant);
    int diffs = 0;
    int checks = 0;
    for (const auto& v : vectors) {
        ++checks;
        const auto a = runUpstream(up, v);
        const auto b = runPort(v);
        if (a != b) {
            ++diffs;
            std::fprintf(stderr, "DIFF line %d %s [%s]: upstream %s, port %s\n", v.line, v.func.c_str(), argv[2],
                         show(a).c_str(), show(b).c_str());
        }
    }
    std::printf("%d rows compared against upstream (%s), %d differences\n", checks, argv[2], diffs);
    return diffs == 0 ? 0 : 1;
}
