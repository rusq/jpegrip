# JPEG the Ripper — Claude Code Project Guide

## Project Overview

**jpegrip** is a forensic/data-recovery CLI tool that extracts JPEG files from arbitrary binary data (disk images, raw dumps, etc.). Originally created in 2005 to recover accidentally deleted photos. Current version: **v0.3.3**.

- Language: **C (C89/C90 standard)** — strictly enforced via `-std=c89 -pedantic-errors`
- License: BSD 3-clause
- No external dependencies (deliberately avoids libjpeg)
- Cross-platform: Linux, macOS (universal binary), Windows, **DOS (Borland C++ 3.0)**

## Compiler Compatibility Requirement: Borland C++ 3.0 (DOS)

**This is a hard constraint.** All code must remain compilable with Borland C++ 3.0 (1992), a 16-bit DOS compiler. This is the strictest target and governs what language features are permissible.

### Implications for code changes

| Constraint | Detail |
|------------|--------|
| **`int` is 16-bit** | Range −32768..32767. Never use `int` to hold file offsets or sizes. |
| **`long` is 32-bit** | Max file addressable via `long`/`fseek`/`ftell` is ~2 GB. |
| **No `stdint.h`** | Not available. Use `unsigned char`, `unsigned long`, etc. explicitly. |
| **No `snprintf`**  | Borland C++ 3.0 does not provide `snprintf`. Use `sprintf` with bounded inputs. |
| **No `//` comments** | C89 only — block comments `/* */` exclusively. |
| **No mixed declarations/code** | All variable declarations must be at the top of their block. |
| **No VLAs, no designated initialisers** | C89 restrictions apply strictly. |
| **Memory model awareness** | DOS uses segmented memory (near/far). Avoid assumptions about pointer size. Keep allocations modest. |
| **No POSIX-only APIs** | `fseek`/`ftell`/`fread`/`fwrite`/`fopen`/`fclose` are safe. Avoid `pread`, `mmap`, etc. |
| **`EOF` value** | Must treat `EOF` as an opaque `int` constant, not assume it equals −1. |
| **Large file defines in `compat.h`** | `_LARGEFILE_SOURCE` etc. are ignored by Borland but must not break compilation. Guard with `#ifndef` or compiler checks if needed. |

### What this means practically

- Do **not** introduce `snprintf` — use `sprintf` with inputs that are statically bounded.
- Do **not** use `int` for sizes, offsets, or file positions — use `long`.
- Do **not** use C99/C11 features even if GCC/Clang allow them under `-std=c89`.
- Do **not** rely on `size_t` being 32-bit; on 16-bit DOS it is 16-bit (max 65535).
- Keep buffer sizes and allocation sizes within 16-bit `size_t` range when possible, or use `unsigned long` explicitly.
- The `BUF_SIZE 16384` in `jpegrip.c` is near the 16-bit `size_t` limit — do not increase it without care.

## Repository Layout

```
jpegrip/
├── main.c          # Entry point, CLI arg parsing, usage text
├── jpegrip.c       # Core ripping logic
├── jpegrip.h       # Public API + JPEGRIP_VERSION constant
├── jpeg.c          # JPEG header parser (thumbnail-aware)
├── jpeg.h          # jpeg_hdr_size() declaration
├── jpeghdr.c       # Standalone diagnostic tool for JPEG header inspection
├── log.c/log.h     # Levelled logging (normal / verbose / trace)
├── compat.h        # Cross-platform defs (large file support, MAX_FNAME=260)
├── Makefile        # Primary build system
├── CMakeLists.txt  # CMake alternative (builds jpegrip + jpeghdr targets)
├── CMakePresets.json
├── build.bat       # Windows build script
├── Dockerfile      # Docker build
├── sample.bin      # Binary test data
├── samples/        # Sample JPEG files for testing
└── mksample.sh     # Script to create sample.bin test data
```

## How It Works

### Detection Algorithm

1. Scan input file for JPEG SOI signature: `FF D8 FF E0 00 10` (JFIF only)
2. Parse JPEG header via `jpeg_hdr_size()` to find the **Quantisation Table (DQT)** marker
   - This skips past APP0, APP1 (EXIF), comments, and — critically — the embedded thumbnail
   - Thumbnails end with their own EOI (`FF D9`), which would confuse naive SOI→EOI extraction
3. Search for EOI (`FF D9`) starting *after* the DQT offset
4. Extract the byte range [SOI..EOI] to a sequentially named file: `jpg00000001.jpg`, `jpg00000002.jpg`, …

### Key Implementation Notes

- **Buffer overlap**: `search_file()` rewinds by `seq_sz` bytes between buffer reads to catch sequences that straddle buffer boundaries (buffer size: 16 KiB)
- **JFIF-only**: The header parser (`jpeg.c`) only handles APP0 (`FF E0`). Files with APP1 (EXIF) as the second marker will fail `jpeg_hdr_size()` and cause `rip_jpeg()` to abort. This is a known limitation.
- `RET_ERROR` is defined as `0` (falsy), `RET_OK` as `1` — inverse of typical C convention; functions are checked with `if (!read_marker(...))`.
- `search_file()` returns the platform `EOF` constant or the internal `ERROR (-2)` / `NOT_FOUND (-3)` sentinels — these are mixed `long` return values, not the same as stdlib `EOF`.

## Building

### macOS (universal binary via lipo)
```sh
make jpegrip
```
Produces both `x86_jpegrip` (x86_64) and `arm_jpegrip` (arm64), then merges with `lipo`.

### Linux
```sh
make jpegrip
```

### Debug build
```sh
make debug jpegrip
```

### Memory leak check (macOS)
```sh
make leaks
```

### CMake
```sh
cmake --preset <preset>   # see CMakePresets.json
cmake --build build/
```
Builds two targets: `jpegrip` and `jpeghdr`.

### Windows
```bat
build.bat
```

### Docker
```sh
make docker
```

## Usage

```
jpegrip [-v|-vv] <input_file>
  -v   verbose mode
  -vv  trace mode (very verbose)
```

Output JPEGs are written to the **current working directory** as `jpg00000000.jpg`, `jpg00000001.jpg`, etc.

## Source File Responsibilities

| File | Responsibility |
|------|---------------|
| `main.c` | `main()`, `usage()`, `run()` — CLI glue |
| `jpegrip.c` | `rip_jpeg()`, `search_file()`, `search_buf()`, `extract()`, `fmt_string()`, `format_name()` |
| `jpeg.c` | `jpeg_hdr_size()` — parses JPEG markers to skip thumbnail data |
| `jpeghdr.c` | Standalone diagnostic: prints header size of a given JPEG |
| `log.c` | `llog()`, `lverbose()`, `ltrace()`, `set_log_level()` |
| `compat.h` | `_LARGEFILE*` defines, `MAX_FNAME` (260) |

## Coding Conventions

- **C89 standard** — no C99/C11 features (no `//` comments, no `_Bool`, no VLAs, no designated initialisers, no `stdint.h`)
- Error labels named `looser:` (intentional misspelling, retained from the original 2005 code)
- `goto looser` used for cleanup paths (single exit point with resource cleanup)
- `snprintf` used for filename generation
- All allocations checked; `malloc` failures go to `looser` label

## Known Limitations / TODO

- Only detects **JFIF** JPEG signatures (`FF D8 FF E0 00 10`); EXIF-only JPEGs (`FF D8 FF E1`) are not found
- `jpeg_hdr_size()` will fail on EXIF-formatted files (APP1 second marker)
- Output filenames are sequential — original filenames are not restored
- TODO (from `TODO` file): implement more file formats beyond JPEG

## Testing

Use `sample.bin` (created by `mksample.sh`) as a test input. Sample JPEG files are in `samples/`.

```sh
./jpegrip sample.bin
./jpegrip -v sample.bin
./jpegrip -vv sample.bin
```
