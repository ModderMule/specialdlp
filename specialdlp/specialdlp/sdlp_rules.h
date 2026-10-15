// Internal declarations of the upstream rule functions (antiLeech.cpp).
// Not exported; the public C ABI lives in include/specialdlp.h.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later version.

#ifndef SDLP_RULES_H
#define SDLP_RULES_H

#include "sdlp_compat.h"

// Upstream build variants (SDC_* defines), selectable at runtime. Exclusive like upstream configs.
enum class SdlpVariant : unsigned {
    Lite = 0,
    AllVeryCD = 1,
    EasyMule = 2,
    VeryCDNickname = 3,
    VeryCDTag = 4,
};

SdlpVariant sdlpVariant() noexcept;
void sdlpSetVariant(SdlpVariant v) noexcept;

#define SDC_VARIANT(v) (sdlpVariant() == SdlpVariant::v)

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif
const DWORD GetDLPVersion(void);     // upstream signature, keep the const
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
LPCTSTR DLPCheckUserhash(const PBYTE userhash);
LPCTSTR DLPCheckModstring_Hard(LPCTSTR modversion, LPCTSTR clientversion);
LPCTSTR DLPCheckModstring_Soft(LPCTSTR modversion, LPCTSTR clientversion);
LPCTSTR DLPCheckUsername_Hard(LPCTSTR username);
LPCTSTR DLPCheckUsername_Soft(LPCTSTR username);
LPCTSTR DLPCheckNameAndHashAndMod(const CString username, const CString& userhash, const CString& modversion);
LPCTSTR DLPCheckMessageSpam(LPCTSTR messagetext);
LPCTSTR DLPCheckHelloTag(const UINT tagnumber);
LPCTSTR DLPCheckInfoTag(const UINT tagnumber);

#endif // SDLP_RULES_H
