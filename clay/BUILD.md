# Building Clay Kernel (WASM)

Clay's native kernel wraps OpenCASCADE (OCCT) and compiles to WASM via Emscripten.

## Prerequisites

1. **Emscripten SDK** — in `toolchain/emsdk/` (or set `EMSDK` env var)
2. **OpenCASCADE WASM build** — in `toolchain/opencascade-build-wasm/` (or set `OPENCASCADE_WASM_ROOT` env var)
3. **OpenCASCADE source** — in `toolchain/opencascade-src/` (needed for headers referenced by the WASM build)

The OCCT WASM headers contain hardcoded include paths to `deps/opencascade-src/`. If `deps/` doesn't exist, create a junction:

```
cd <repo-root>
mklink /J deps\opencascade-src toolchain\opencascade-src   # Windows
ln -s toolchain/opencascade-src deps/opencascade-src       # macOS/Linux
```

## Build

From the repo root:

```bash
export EMSDK="<repo>/toolchain/emsdk"
export OPENCASCADE_WASM_ROOT="<repo>/toolchain/opencascade-build-wasm"
stone extern build --package slabs/clay --verbose
```

Or equivalently:

```bash
cd slabs/clay
stone extern build --verbose
```

This runs Emscripten (`emcc`) to compile `kernel/clay_kernel.c` + `kernel/occt_wrapper.cpp` against the OCCT static libraries, producing `kernel/builds/wasm/clay_kernel.wasm`.

The build command auto-derives exported functions from `.stn` import statements — no manual export list needed.

## Output

```
slabs/clay/kernel/builds/wasm/clay_kernel.wasm   (~20 MB)
```

## Publishing

```bash
basin login                           # authenticate (one-time)
cd slabs/clay
basin publish --org stone-slabs       # publishes to Basin registry
```

## How it works

`stone extern build` (in `packages/stone/cli/commands/extern.js`):

1. Reads `package.son` → finds `extern = { type = "c", name, sources, deps }`
2. Scans local `.stn` files for `import ... from _clay_kernel` statements
3. Parses C function signatures to determine return types (ptr, num, str)
4. Builds `EXPORTED_FUNCTIONS` list (ABI exports + kernel exports)
5. Resolves dependency paths from env vars (`OPENCASCADE_WASM_ROOT`)
6. Invokes `emcc` with OCCT static libs, shared memory flags, and `-O2`
