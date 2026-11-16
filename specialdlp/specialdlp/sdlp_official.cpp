// Ghost mod rules, see sdlp_official.h. Lists from official eMule 0.70b
// (BaseClient.cpp SendHelloTypePacket / ProcessHelloTypePacket); older official
// versions send a subset with lower field values.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later version.

#include "sdlp_official.h"

namespace {

constexpr UINT makeRawVersion(UINT mjr, UINT min, UINT upd)
{
    return (mjr << 17) | (min << 10) | (upd << 7);
}

// eMule v0.70b; raise with each official release that changes the hello
constexpr UINT kVersionMax = makeRawVersion(0, 70, 1);

// hello tags official eMule reads (CT_MOD_VERSION carries the modstring, empty is fine)
constexpr UINT kKnownHelloTags[] = {
    0x01, // CT_NAME
    0x0f, // CT_PORT
    0x11, // CT_VERSION
    0x55, // CT_MOD_VERSION
    0xf9, // CT_EMULE_UDPPORTS
    0xfa, // CT_EMULE_MISCOPTIONS1
    0xfb, // CT_EMULE_VERSION
    0xfc, // CT_EMULE_BUDDYIP
    0xfd, // CT_EMULE_BUDDYUDP
    0xfe, // CT_EMULE_MISCOPTIONS2
    0xef, // CT_EMULECOMPAT_OPTIONS1, in official opcodes.h, sent by aMule
    // IPv6 extension, additive and opt-in, carries no mod evidence
    0xa0, // CT_EMULE_SERVINGBUDDYIPV6
    0xad, // CT_MOD_YOUR_IP
    0xae, // CT_MOD_IP_V6
};
// on purpose not here: 0x3d ET_INCOMPLETEPARTS, 0x69/0x6a webcache, 0xee L2L, 0x3e L2HAC,
// 0xa1-0xac/0xaf/0xbf mod tags; MiscOptions2 bit 6 is the mod-protocol bit

// OP_EMULEINFO tags official eMule reads (old clients send them instead of the hello extras)
constexpr UINT kKnownInfoTags[] = {
    0x20, // ET_COMPRESSION
    0x21, // ET_UDPPORT
    0x22, // ET_UDPVER
    0x23, // ET_SOURCEEXCHANGE
    0x24, // ET_COMMENTS
    0x25, // ET_EXTENDEDREQUEST
    0x26, // ET_COMPATIBLECLIENT
    0x27, // ET_FEATURES
    0x55, // ET_MOD_VERSION
    0x94, // ET_OS_INFO, sent by aMule
};
// on purpose not here: 0x3d ET_INCOMPLETEPARTS

// a bit field and the highest value official eMule sends in it
struct Field {
    UINT shift;
    UINT width;
    UINT max;
};

// all 32 bits are defined; versions above these are unknown
constexpr Field kMiscOptions1[] = {
    {29, 3, 1}, // AICH
    {24, 4, 4}, // UDP
    {20, 4, 1}, // data compression
    {16, 4, 3}, // secure ident
    {12, 4, 4}, // source exchange 1
    {8, 4, 2},  // extended requests
    {4, 4, 1},  // comments
};

// bits 0-3 Kad, 4 large files, 5 ext multipacket, 7-9 crypt, 10 SX2, 11 captcha,
// 12 direct callback, 13 file ident; 6 is the reserved mod bit, 14-31 reserved
constexpr UINT kMiscOptions2Known = 0x00003FBFu;
constexpr Field kMiscOptions2[] = {
    {0, 4, 0x0a}, // KADEMLIA_VERSION
};

// mjr/min/upd only; build (bits 0-6) is never set
constexpr UINT kEmuleVersionKnown = 0x00FFFF80u;

// whole-value OP_EMULEINFO tags and their highest official value
struct InfoMax {
    UINT tag;
    UINT max;
};
constexpr InfoMax kInfoMax[] = {
    {0x20, 1},       // ET_COMPRESSION
    {0x21, 0xFFFF},  // ET_UDPPORT
    {0x22, 4},       // ET_UDPVER
    {0x23, 4},       // ET_SOURCEEXCHANGE
    {0x24, 1},       // ET_COMMENTS
    {0x25, 2},       // ET_EXTENDEDREQUEST
};
// ET_FEATURES: bits 0-1 secure ident, 7 preview
constexpr UINT kInfoFeaturesKnown = 0x83u;

template <size_t N>
bool known(UINT tagnumber, const UINT (&tags)[N])
{
    for (const UINT t : tags) {
        if (t == tagnumber)
            return true;
    }
    return false;
}

bool official(UINT emuleversion)
{
    return (emuleversion >> 24) == 0 && (emuleversion & 0x00FFFF80u) <= kVersionMax;
}

template <size_t N>
bool fieldsOk(UINT value, const Field (&fields)[N])
{
    for (const Field& f : fields) {
        if (((value >> f.shift) & ((1u << f.width) - 1u)) > f.max)
            return false;
    }
    return true;
}

} // namespace

UINT OfficialVersionMax()
{
    return kVersionMax;
}

LPCTSTR OfficialCheckHelloTag(UINT tagnumber, UINT emuleversion)
{
    return official(emuleversion) && !known(tagnumber, kKnownHelloTags) ? _T("Ghost Mod: unknown hello tag") : nullptr;
}

LPCTSTR OfficialCheckHelloBits(UINT tagnumber, UINT value, UINT emuleversion)
{
    if (!official(emuleversion))
        return nullptr;
    switch (tagnumber) {
    case 0x0f: // CT_PORT
    case 0xfd: // CT_EMULE_BUDDYUDP
        return value > 0xFFFFu ? _T("Ghost Mod: unknown bits in port tag") : nullptr;
    case 0xfa:
        return fieldsOk(value, kMiscOptions1) ? nullptr : _T("Ghost Mod: unknown bits in MiscOptions1");
    case 0xfe:
        return (value & ~kMiscOptions2Known) == 0 && fieldsOk(value, kMiscOptions2)
                   ? nullptr
                   : _T("Ghost Mod: unknown bits in MiscOptions2");
    case 0xfb:
        return (value & ~kEmuleVersionKnown & 0x00FFFFFFu) == 0 ? nullptr
                                                                : _T("Ghost Mod: unknown bits in eMule version");
    default:
        return nullptr;
    }
}

LPCTSTR OfficialCheckInfoTag(UINT tagnumber, UINT emuleversion)
{
    return official(emuleversion) && !known(tagnumber, kKnownInfoTags) ? _T("Ghost Mod: unknown info tag") : nullptr;
}

LPCTSTR OfficialCheckInfoBits(UINT tagnumber, UINT value, UINT emuleversion)
{
    if (!official(emuleversion))
        return nullptr;
    if (tagnumber == 0x27)
        return (value & ~kInfoFeaturesKnown) == 0 ? nullptr : _T("Ghost Mod: unknown bits in info features");
    for (const InfoMax& m : kInfoMax) {
        if (m.tag == tagnumber)
            return value > m.max ? _T("Ghost Mod: unknown info tag value") : nullptr;
    }
    return nullptr;
}
