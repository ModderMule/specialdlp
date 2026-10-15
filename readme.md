﻿Strict DLP Chinese (SDC)
=====

<p align="center">
<a href="https://github.com/chengr28/specialdlp/blob/master/license"><img src="https://img.shields.io/github/license/chengr28/specialdlp"></a> <a href="https://github.com/chengr28/specialdlp/releases/latest"><img src="https://img.shields.io/github/v/release/chengr28/specialdlp"></a>
</p>

Anti-leecher set based on the Xtreme mod (DLP) and the strict Chinese set (SDC). Modified for Qt to run on all platforms.

<p align="center">
<a href="readme.zh-hans.md">简体中文介绍</a> | <a href="readme.zh-hant.md">繁體中文介紹</a>
</p>

* [Feature table for SDC variants](https://github.com/chengr28/specialdlp/blob/master/specialdlp/document/readme.en.md)
* [Download a single SDC variant](https://github.com/chengr28/specialdlp/tree/binary)
* [Releases page: Download all SDC variants in a signle package](https://github.com/chengr28/specialdlp/releases)
* [FAQs and other tips](https://github.com/chengr28/specialdlp/blob/master/specialdlp/document/readme.en.txt)
* [Changelog](https://github.com/chengr28/specialdlp/blob/master/specialdlp/document/changelog.en.txt)
* [Checksum](https://github.com/chengr28/specialdlp/blob/master/specialdlp/document/checksum.md)

## Portable fork

This fork ([ModderMule/specialdlp](https://github.com/ModderMule/specialdlp)) builds the same SDC rules as a
cross-platform shared library (Windows, macOS, Linux; x64 and arm64) for [eMuleQt](https://emule-qt.org).
The rule code in `specialdlp/specialdlp/antiLeech.cpp` stays as close to upstream as possible so upstream
updates merge cleanly. ATL/shlwapi are replaced by `sdlp_compat.h`.

* **C ABI:** see [`include/specialdlp.h`](include/specialdlp.h).
  * UTF-16 strings and `extern "C"` functions with an `sdlp_` prefix.
  * The ABI version is checked at load.
  * Each check returns NULL for a clean client, or a reason string in static storage.
* **Variants:** one binary for all of them. Select one at runtime with `sdlp_set_variant()` (lite, all-verycd,
  easymule, verycd-nickname or verycd-tag), instead of shipping one DLL per variant.
* **Not compatible with the Xtreme `antiLeech.dll` exports:** `DLPCheckNameAndHashAndMod` took MFC `CString`,
  which cannot cross a portable ABI.
* **Unbuilt leftovers:** `specialdlpTester` (Windows/MFC) stays in the tree as a reference, but is no longer
  built.

Build and test:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Test coverage:
* `tests/vectors.tsv` holds positive and negative cases for every rule.
* On Windows, CI also builds the original upstream DLLs and checks that the port returns the same result for
  every vector, for each variant.
* Tagged builds (`v*`) publish per-platform archives plus `SHA256SUMS.txt`.

## External Links

* [Project in GitLab](https://gitlab.com/chengr28/specialdlp)
* [Project in Sourceforge](https://sourceforge.net/projects/specialdlp)
* [SDC in eMuleFans 电骡爱好者](https://emulefans.com/news/plugin/dlp/sdc)
