English | [中文](/README_cn.md)

[![License](https://img.shields.io/github/license/mashape/apistatus.svg)](LICENSE)

**cppJSON** is a lightweight, easy-to-use C++ JSON library, evolved from [neb::CJsonObject](https://github.com/Bwar/CJsonObject), built on top of cJSON v1.7. Key features:

- **Exact 64-bit integers**: full-range `int64`/`uint64` exact parse, generate, read
- **STL container interop**: `vector`/`map`/`list`/`set` ↔ JSON, nested supported
- **Map-style assignment**: `o["k"] = value` auto-creates the key
- **ArduinoJson-style defaults**: `o["k"] | defval`, `o.Get("k", defval)`
- **O(n) array traversal**: `GetNextValue()`, Ryu fast float printing
- **RAII**: auto memory, no third-party runtime deps, embedded-friendly

For the full API reference with examples, see [API_GUIDE.md](/API_GUIDE.md).

---

## Features

Based on official cJSON v1.7, with enhancements:

| Feature | Description |
|---------|-------------|
| **Exact 64-bit integers** | `int64`/`uint64` full-range exact parse, generate, read (official cJSON stores numbers as `double`) |
| **Map-style assignment** | `o["k"] = value` auto-creates the key, nested `o["a"]["b"] = v` supported |
| **Default-value read** | ArduinoJson-style `operator\|`, side-effect-free `o.Get("k", defval)` |
| **STL containers** | `vector`/`list`/`deque`/`set` ↔ array, `map`/`unordered_map` ↔ object, nested containers supported |
| **O(n) array traversal** | `GetNextValue()` (a `Get(i)` loop is O(n²)) |
| **Ryu float printing** | shortest & exact, bit-exact round-trip, ~10× faster than `%g` |
| **Lightweight RAII** | no third-party deps, auto memory management, embedded-friendly |

## Performance

Compared to mainstream C++ JSON libraries (1.2 MB payload, `-O2`, MinGW x86_64):

| Library | Parse | Dump | Build | Nested traversal |
|---------|-------|------|-------|------------------|
| **cppJSON** | **51** MB/s | **143** MB/s | **9** ns/elem ✅ | 490 ns/elem |
| nlohmann/json | 40 MB/s | 132 MB/s | 90 ns/elem | 244 ns/elem |
| RapidJSON | 309 MB/s | 434 MB/s | 16 ns/elem | 10 ns/elem |
| simdjson | 763 MB/s | — | — | 1.7 ns/elem |

**✅ = fastest overall (build 9 ns/elem, ~1.8× faster than RapidJSON, ~10× than nlohmann)**

**Bold** = cppJSON wins within its tier (classic per-char parser):
- **Parse 51 MB/s > nlohmann 40 MB/s**
- **Dump 143 MB/s > nlohmann 132 MB/s**

Positioning:

- **Build** is the strongest point: fastest overall.
- **Dump**, after the Ryu optimization, exceeds nlohmann.
- **Parse** is at the classic cJSON parser level (faster than nlohmann; RapidJSON/simdjson use SIMD batch parsing).
- Requires C++11 or later (STL containers, `operator|`, `nullptr` features depend on it).

## Project origin

This project (`cppJSON`) is **derived from [Bwar/CJsonObject](https://github.com/Bwar/CJsonObject)** (formerly `neb::CJsonObject`). Thanks to the original author Bwar for the open-source contribution.

Main changes vs. the original:

- Class `neb::CJsonObject` → global **`cppJSON`** (the `neb` namespace was removed)
- Upgraded & extended the underlying cJSON (exact 64-bit integers, Ryu float printing)
- Added STL container interop, map-style assignment, `operator|` default-value reads, etc.

## Fork statement

The cJSON used by this project is **forked from [DaveGamble/cJSON](https://github.com/DaveGamble/cJSON) (official v1.7.19)**, MIT licensed, with the original copyright notice retained.

**cJSON is not vendored**: it is fetched at build time (official v1.7.19) and our patch `patches/cJSON_v1.7.19_cppJSON.patch` (all extensions) is applied automatically. See [API_GUIDE.md](/API_GUIDE.md) §1.

Backward-compatible extensions on top of the official version (official API unaffected):

- `cJSON.valueint` widened to `int64_t` for exact 64-bit integers (official cJSON stores numbers as `double`)
- New `cJSON.sign` field (signed/unsigned marker for integers)
- New `cJSON_CreateInt64()` / `cJSON_CreateUint64()` APIs
- Float printing via [Ryu](https://github.com/ulfjack/ryu) shortest-exact algorithm (~10× faster)
- Parser tuning (number-literal scanning, etc.)

cppJSON is an independently-evolved wrapper over cJSON; both work standalone — the bundled `cJSON.c/h` can be compiled on their own. This project is maintained independently on GitHub (issue/PR trackers are separate from cJSON).

## Third-party dependencies

| Dependency | License | Purpose |
|------------|---------|---------|
| [cJSON](https://github.com/DaveGamble/cJSON) | MIT | JSON parse / serialize core |
| [Ryu](https://github.com/ulfjack/ryu) | Apache-2.0 OR Boost-1.0 | shortest & exact double formatting (fetched at build time, pinned commit `4c0618b`) |

Full license texts in [THIRD_PARTY_LICENSES.md](/THIRD_PARTY_LICENSES.md).
