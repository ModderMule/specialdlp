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
