/*
 * specialdlp - portable Dynamic Leecher Protection (DLP) library.
 *
 * Stable C ABI, meant to be loaded at runtime (dlopen/LoadLibrary/QLibrary) so the
 * rule set can be updated independently of the host application.
 *
 * Strings: UTF-16, NUL-terminated (sdlp_char16 = char16_t in C++). Every check returns NULL for a clean
 * client, otherwise a reason string in static storage. The pointer stays valid while
 * the library is loaded; hosts should copy it immediately. NULL inputs are clean.
 *
 * Thread safety: all functions are reentrant. sdlp_set_variant() is atomic.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 */

#ifndef SPECIALDLP_H
#define SPECIALDLP_H

#include <stdint.h>

#if defined(_WIN32)
#  if defined(SDLP_BUILDING)
#    define SDLP_API __declspec(dllexport)
#  else
#    define SDLP_API __declspec(dllimport)
#  endif
#else
#  define SDLP_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
typedef char16_t sdlp_char16;
extern "C" {
#else
typedef uint_least16_t sdlp_char16; /* no <uchar.h> on every platform */
#endif

/* Bumped on any incompatible change of this header. Hosts must refuse a mismatch. */
#define SDLP_ABI_VERSION 1u

/* Upstream build variants, exclusive. */
#define SDLP_VARIANT_LITE             0u
#define SDLP_VARIANT_ALL_VERYCD       1u
#define SDLP_VARIANT_EASYMULE         2u
#define SDLP_VARIANT_VERYCD_NICKNAME  3u
#define SDLP_VARIANT_VERYCD_TAG       4u

SDLP_API uint32_t sdlp_abi_version(void);
SDLP_API uint32_t sdlp_rules_version(void);   /* upstream DLPVERSION, e.g. 44006 */
SDLP_API const char* sdlp_lib_version(void);  /* library semver, e.g. "1.0.0" */

/* Returns 0 on success, -1 for an unknown variant (state unchanged). */
SDLP_API int32_t sdlp_set_variant(uint32_t variant);
SDLP_API uint32_t sdlp_get_variant(void);

/* Hard = ban. Soft = ban or reduce score (host decides). Call order as in eMule Xtreme:
 * hello tag, info tag, modstring hard, modstring soft, userhash, name+hash+mod,
 * username soft, username hard; message spam on first chat message. */
SDLP_API const sdlp_char16* sdlp_check_modstring_hard(const sdlp_char16* modversion, const sdlp_char16* clientversion);
SDLP_API const sdlp_char16* sdlp_check_modstring_soft(const sdlp_char16* modversion, const sdlp_char16* clientversion);
SDLP_API const sdlp_char16* sdlp_check_username_hard(const sdlp_char16* username);
SDLP_API const sdlp_char16* sdlp_check_username_soft(const sdlp_char16* username);
/* userhash_hex: 32 hex chars (any case). */
SDLP_API const sdlp_char16* sdlp_check_name_hash_mod(const sdlp_char16* username, const sdlp_char16* userhash_hex,
                                                  const sdlp_char16* modversion);
SDLP_API const sdlp_char16* sdlp_check_message_spam(const sdlp_char16* message);
/* userhash: 16 raw bytes. */
SDLP_API const sdlp_char16* sdlp_check_userhash(const uint8_t* userhash);
SDLP_API const sdlp_char16* sdlp_check_hello_tag(uint32_t tag);
SDLP_API const sdlp_char16* sdlp_check_info_tag(uint32_t tag);

#ifdef __cplusplus
}
#endif

#endif /* SPECIALDLP_H */
