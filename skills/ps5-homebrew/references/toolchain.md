# The toolchain

## The compiler

Titles build for `x86_64-sie-ps5` with the host's clang, driven by PS5_Vulkan's
wrapper `tooling/prospero-clang18`:

```bash
PS5_PAYLOAD_SDK=<sdk> PS5_CLANG=$(command -v clang) sh ../PS5_Vulkan/tooling/prospero-clang18 -c ...
```

It adds the target, the SDK as sysroot (its libc and libc++ headers), and four
flags that are not optional:

| Flag | Why |
| --- | --- |
| `-femulated-tls` | the console's thread-local storage is emulated; `__emutls_v.*` in an object is also the proof it was built for the target |
| `-fno-plt`, `-fno-stack-protector` | what the console's loader and libc expect |
| `-fdenormal-fp-math=ieee` | the compiler must not assume the FTZ/DAZ state a title starts in (`platform-contracts.md`) |

Add, in the project's flags:

- **`-fno-omit-frame-pointer`.** Console backtraces are frame-pointer walks; without
  frame pointers there is no backtrace at all (`ps5-console` skill, crash reading).
- **`-ffunction-sections -fdata-sections`.** The link is large, and these keep it
  smaller.

The SDK comes from the fork at a pinned revision. Each title pins its own and installs
it into its `.deps/native/ps5-payload-sdk`: a title made with `new-title.py` in
`ps5/tools/setup-sdk.sh` (`sdk_revision`), PS5_RetroArch and PS5_vkQuake in
`tools/setup-native-dependencies.sh`. The RADV archive comes from PS5_Vulkan, built
with that project's own pin; a title may pin a later SDK revision, as long as the
platform layer keeps what the archive was linked against (the link fails if not).

## Libraries

Build third-party code for the target into static archives. The SDK fork ships
target wrappers in `<sdk>/bin/`: `prospero-clang`, `prospero-clang++`, `prospero-ar`,
`prospero-ranlib` (the fork's versions assume IEEE denormals, like the wrapper above).

- **CMake**: a toolchain file. PS5_RetroArch's `tooling/azahar/ps5-toolchain.cmake`
  is a working one:

  ```cmake
  set(CMAKE_SYSTEM_NAME FreeBSD)
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
  set(CMAKE_C_COMPILER "$ENV{PS5_PAYLOAD_SDK}/bin/prospero-clang")
  set(CMAKE_CXX_COMPILER "$ENV{PS5_PAYLOAD_SDK}/bin/prospero-clang++")
  set(CMAKE_AR "$ENV{PS5_PAYLOAD_SDK}/bin/prospero-ar")
  set(CMAKE_RANLIB "$ENV{PS5_PAYLOAD_SDK}/bin/prospero-ranlib")
  set(CMAKE_FIND_ROOT_PATH "$ENV{PS5_PAYLOAD_SDK}")
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  ```

  Build static libraries (`-DBUILD_SHARED_LIBS=OFF`), turn off the project's tests
  and tools, and add `-fno-omit-frame-pointer`.
- **Meson**: a cross file with the same compilers. PS5_Vulkan's `tools/build-radv.sh`
  builds Mesa with `tooling/radv/ps5-cross.ini`.
- **Autotools**: `--host=x86_64-unknown-freebsd14` with `CC`/`CXX`/`AR` set to the
  wrappers, `--disable-shared --enable-static` (PS5_RetroArch's
  `tools/build-libiconv.sh`). FFmpeg's own configure takes `--enable-cross-compile
  --target-os=freebsd --arch=x86_64 --cpu=znver2` (`tools/build-ffmpeg.sh`): the
  console's CPU is Zen 2.

A library that probes the host at configure time (`try_run`, feature tests that
execute) needs its answers supplied, because nothing built for the console runs on
the build host.

## Linking

The link recipe and the packaging steps are in `vulkan-on-radv.md` and
`title-packaging.md`. Two linker facts that cause most "it builds but does nothing"
reports:

- **An archive member referenced only weakly, or only from a constructor, is
  dropped.** If a component seems not to run, check whether the linker kept it
  before debugging its logic (`--whole-archive` for that archive, or a strong
  reference).
- **Order matters.** SDK libraries and archives that need each other go inside one
  `--start-group ... --end-group`.

## When a miscompile is suspected

Establish which side is wrong before changing flags. Compile the same input for
the host, run it there, and compare. A fault that reproduces only on the console is
a fact about the console build (FP state, TLS, the libc in use), not proof that the
source is wrong. Check `platform-contracts.md` ("Floating-point state") first: it is
the commonest cause of numbers that differ from a desktop.
