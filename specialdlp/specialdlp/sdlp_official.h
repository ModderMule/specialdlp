// Ghost mod rules: an official-looking eMule (no modstring, compatible client 0) that sends
// hello tags or bits its claimed version never sent. Own rules, not upstream; kept in their
// own file so upstream antiLeech.cpp merges stay clean.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later version.

#ifndef SDLP_OFFICIAL_H
#define SDLP_OFFICIAL_H

#include "sdlp_compat.h"

// Newest official release the lists cover, raw CT_EMULE_VERSION form. Newer = clean.
UINT OfficialVersionMax();

// emuleversion: raw CT_EMULE_VERSION value. NULL = clean.
LPCTSTR OfficialCheckHelloTag(UINT tagnumber, UINT emuleversion);
LPCTSTR OfficialCheckHelloBits(UINT tagnumber, UINT value, UINT emuleversion);
// same for OP_EMULEINFO; emuleversion = compatible client << 24 | hello CT_EMULE_VERSION (0 if none)
LPCTSTR OfficialCheckInfoTag(UINT tagnumber, UINT emuleversion);
LPCTSTR OfficialCheckInfoBits(UINT tagnumber, UINT value, UINT emuleversion);

#endif // SDLP_OFFICIAL_H
