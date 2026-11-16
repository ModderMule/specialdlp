// Runs tests/vectors.tsv against the specialdlp C ABI, plus a few API edge cases.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later version.

#include "specialdlp.h"
#include "vectors.h"

#include <cstdio>
#include <cstring>

namespace {

int g_failures = 0;

void fail(const std::string& msg)
{
    ++g_failures;
    std::fprintf(stderr, "FAIL: %s\n", msg.c_str());
}

std::optional<std::u16string> wrap(const sdlp_char16* r)
{
    return r ? std::optional<std::u16string>(r) : std::nullopt;
}

std::optional<std::u16string> run(const sdlp::test::Vector& v)
{
    const auto& a = v.args;
    if (v.func == "modhard")
        return wrap(sdlp_check_modstring_hard(a[0].c_str(), a[1].c_str()));
    if (v.func == "modsoft")
        return wrap(sdlp_check_modstring_soft(a[0].c_str(), a[1].c_str()));
    if (v.func == "userhard")
        return wrap(sdlp_check_username_hard(a[0].c_str()));
    if (v.func == "usersoft")
        return wrap(sdlp_check_username_soft(a[0].c_str()));
    if (v.func == "namehashmod")
        return wrap(sdlp_check_name_hash_mod(a[0].c_str(), a[1].c_str(), a[2].c_str()));
    if (v.func == "spam")
        return wrap(sdlp_check_message_spam(a[0].c_str()));
    if (v.func == "hellotag")
        return wrap(sdlp_check_hello_tag(sdlp::test::parseHex(a[0])));
    if (v.func == "infotag")
        return wrap(sdlp_check_info_tag(sdlp::test::parseHex(a[0])));
    if (v.func == "userhash") {
        const auto hash = sdlp::test::parseHash(a[0]);
        return wrap(sdlp_check_userhash(hash.data()));
    }
    fail("unknown func " + v.func);
    return std::nullopt;
}

std::string show(const std::optional<std::u16string>& s)
{
    return s ? "\"" + sdlp::test::toUtf8(*s) + "\"" : "NULL";
}

void officialTests();

void apiTests()
{
    if (sdlp_abi_version() != SDLP_ABI_VERSION)
        fail("abi version");
    if (sdlp_rules_version() < 44006)
        fail("rules version");
    if (std::strlen(sdlp_lib_version()) < 5)
        fail("lib version");
    if (sdlp_set_variant(99) != -1 || sdlp_get_variant() != SDLP_VARIANT_LITE)
        fail("invalid variant must be rejected");
    if (sdlp_set_variant(SDLP_VARIANT_VERYCD_TAG) != 0 || sdlp_get_variant() != SDLP_VARIANT_VERYCD_TAG)
        fail("set variant");
    sdlp_set_variant(SDLP_VARIANT_LITE);

    // NULL inputs are clean
    if (sdlp_check_modstring_hard(nullptr, u"eMule") || sdlp_check_username_hard(nullptr)
        || sdlp_check_message_spam(nullptr) || sdlp_check_userhash(nullptr)
        || sdlp_check_name_hash_mod(nullptr, nullptr, nullptr))
        fail("NULL input must be clean");

    // same reason pointer on repeat calls (static storage)
    const auto* r1 = sdlp_check_hello_tag(0x15);
    const auto* r2 = sdlp_check_hello_tag(0x15);
    if (r1 == nullptr || r1 != r2)
        fail("reason pointer must be stable");

    // non-BMP chars must not confuse the checks
    if (sdlp_check_username_hard(u"\U0001F600 normal user") != nullptr)
        fail("emoji user must be clean");

    officialTests();
}

// ghost mod rules (sdlp_official.cpp), not upstream: kept out of vectors.tsv for the diff harness
void officialTests()
{
    constexpr auto ver = [](uint32_t mjr, uint32_t min, uint32_t upd) { return (mjr << 17) | (min << 10) | (upd << 7); };
    const uint32_t v70b = ver(0, 70, 1);
    if (sdlp_official_version_max() != v70b)
        fail("official version max");

    // what 0.70b sends
    const uint32_t opts1 = (1u << 29) | (1u << 28) | (4u << 24) | (1u << 20) | (3u << 16) | (4u << 12) | (2u << 8)
                         | (1u << 4) | 0x7u;
    const uint32_t opts2 = 0x0au | (1u << 4) | (1u << 5) | (7u << 7) | (0xFu << 10);
    for (const uint32_t tag :
         {0x01u, 0x0fu, 0x11u, 0x55u, 0xf9u, 0xfau, 0xfbu, 0xfcu, 0xfdu, 0xfeu, 0xefu, 0xa0u, 0xadu, 0xaeu}) {
        if (sdlp_check_official_hello_tag(tag, v70b))
            fail("official tag must be clean");
    }
    if (sdlp_check_official_hello_bits(0xfa, opts1, v70b) || sdlp_check_official_hello_bits(0xfe, opts2, v70b)
        || sdlp_check_official_hello_bits(0xfb, v70b, v70b) || sdlp_check_official_hello_bits(0xfd, 0xFFFF, v70b)
        || sdlp_check_official_hello_bits(0xf9, 0xFFFFFFFFu, v70b))
        fail("official bits must be clean");
    if (sdlp_check_official_hello_bits(0xfa, (1u << 31) | (1u << 29), ver(0, 49, 0)) == nullptr) // AICH 5
        fail("old version must be checked");

    // ghost mod hits
    if (sdlp_check_official_hello_tag(0xaa, v70b) == nullptr || sdlp_check_official_hello_tag(0, v70b) == nullptr
        || sdlp_check_official_hello_tag(0x3d, v70b) == nullptr)
        fail("unknown tag must hit");
    if (sdlp_check_official_hello_bits(0xfa, opts1 | (2u << 4), v70b) == nullptr) // comments 3
        fail("comment version 3 must hit");
    if (sdlp_check_official_hello_bits(0xfe, opts2 | (1u << 6), v70b) == nullptr
        || sdlp_check_official_hello_bits(0xfe, opts2 | (1u << 20), v70b) == nullptr
        || sdlp_check_official_hello_bits(0xfe, (opts2 & ~0xFu) | 0x0b, v70b) == nullptr)
        fail("reserved MiscOptions2 bits must hit");
    if (sdlp_check_official_hello_bits(0xfb, v70b | 5, v70b) == nullptr)
        fail("build bits must hit");
    if (sdlp_check_official_hello_bits(0xfd, 0x10000, v70b) == nullptr)
        fail("port high bits must hit");

    // future official versions and compatible clients pass
    const uint32_t v70c = ver(0, 70, 2);
    const uint32_t v71a = ver(0, 71, 0);
    if (sdlp_check_official_hello_tag(0xaa, v70c) || sdlp_check_official_hello_tag(0xaa, v71a)
        || sdlp_check_official_hello_bits(0xfe, opts2 | (1u << 20), v71a)
        || sdlp_check_official_hello_tag(0xaa, (3u << 24) | v70b))
        fail("newer version or compatible client must be clean");

    // OP_EMULEINFO, what 0.70b sends; 0 = no hello version (old client)
    for (const uint32_t v : {v70b, 0u}) {
        for (const uint32_t tag : {0x20u, 0x21u, 0x22u, 0x23u, 0x24u, 0x25u, 0x26u, 0x27u, 0x55u, 0x94u}) {
            if (sdlp_check_official_info_tag(tag, v))
                fail("official info tag must be clean");
        }
    }
    if (sdlp_check_official_info_bits(0x20, 1, v70b) || sdlp_check_official_info_bits(0x21, 0xFFFF, v70b)
        || sdlp_check_official_info_bits(0x22, 4, v70b) || sdlp_check_official_info_bits(0x23, 3, v70b)
        || sdlp_check_official_info_bits(0x24, 1, v70b) || sdlp_check_official_info_bits(0x25, 2, v70b)
        || sdlp_check_official_info_bits(0x27, 0x83, v70b) || sdlp_check_official_info_bits(0x26, 0, v70b))
        fail("official info values must be clean");
    if (sdlp_check_official_info_tag(0x3d, v70b) == nullptr || sdlp_check_official_info_tag(0x99, 0) == nullptr)
        fail("unknown info tag must hit");
    if (sdlp_check_official_info_bits(0x24, 2, v70b) == nullptr || sdlp_check_official_info_bits(0x27, 0x40, v70b) == nullptr
        || sdlp_check_official_info_bits(0x21, 0x10000, v70b) == nullptr)
        fail("info values above official must hit");
    if (sdlp_check_official_info_tag(0x3d, v71a) || sdlp_check_official_info_tag(0x3d, (1u << 24) | v70b)
        || sdlp_check_official_info_bits(0x24, 2, v71a))
        fail("info: newer version or compatible client must be clean");
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s vectors.tsv\n", argv[0]);
        return 2;
    }
    apiTests();

    std::vector<sdlp::test::Vector> vectors;
    std::string error;
    if (!sdlp::test::loadVectors(argv[1], vectors, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 2;
    }

    int checks = 0;
    for (const auto& v : vectors) {
        for (const auto variant : v.variants) {
            sdlp_set_variant(variant);
            const auto got = run(v);
            ++checks;
            if (got != v.expected)
                fail("line " + std::to_string(v.line) + " " + v.func + " ["
                     + std::string(sdlp::test::kVariantNames[variant]) + "]: expected " + show(v.expected)
                     + ", got " + show(got));
        }
    }
    sdlp_set_variant(SDLP_VARIANT_LITE);

    std::printf("%d vector checks, %d failures\n", checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
