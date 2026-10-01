# Loading code: no dlopen

**A title cannot load a module it brought.** Every `dlopen` and
`sceKernelLoadStartModule` of a repository-built `.so` is refused by the console.
That shapes every port with plugins, drivers or cores. Two answers, in order of
preference.

## 1. Link it in

The default. The Vulkan driver itself arrives this way: RADV is a static archive
linked whole into the title. For a program with plugins:

- build each plugin as a static archive and link the ones the title needs;
- replace the program's `dlopen`/`dlsym` registry with a table of the linked entry
  points, behind `__PROSPERO__`;
- watch for code that only constructors or weak references reach: the linker drops
  it (`--whole-archive` for that archive).

## 2. An in-process ELF loader (PS5_RetroArch's cores)

When the set of modules is large or chosen at run time (RetroArch's libretro cores),
PS5_RetroArch loads them itself: `src/core_loader_ps5.cpp` is a bounded in-process
ELF loader. What it took, so the next one does not rediscover it:

- **Cores are built for the loader**, not as console modules: linked with
  `tooling/native/ps5-core.ld`, with no libc or C++ runtime of their own, and a
  core-local destructor registry (`tooling/native/core_cxx_runtime.cpp`).
- **Their imports bind to the title's own imports.** The title cannot gain imports at
  run time, so its import table is built from the cores it ships with
  (`tools/core-imports.py`, `src/core_imports_ps5.cpp`). A core built on its own
  cannot load into a title that was not linked with its imports. That is also why
  a release title cannot load an RPCS3 core built separately.
- **Code lives in direct memory**, loaded read-write and then given execute through
  `ps5platform/exec.h` (a 424 MiB MAME core does not fit in flexible memory).
- **C++ exceptions in cores**: their `.eh_frame` sections are registered with the
  title's unwinder (`__unw_add_dynamic_eh_frame_section`), and their `_Unwind_*` and
  `__cxa_*` imports are bound to the title's libunwind and libc++abi.
- **Limits it states**: no thread-local storage in cores, no general
  exception-unwind registration beyond that, and it waits for a core's threads to end
  before unmapping it.
- **Every core is checked at build time** (`tools/check-core.py` confirms it is a
  console libretro ELF, not a host library), and a rejected load recovers to the
  menu instead of killing the title.

Copy that loader rather than writing another. Changes that make it more general
belong in PS5_RetroArch first.

## Payloads are different

A **payload** (ps5vkctl, websrv, elfldr) is an ELF the console's payload loader runs
in another context, not a title. Payloads are built with the upstream SDK's payload
flow (`<sdk>/samples`), and none of the title rules about `sce_sys/`, signing or the
shell exit apply. Tools that need to stay resident (launch titles, serve files) are
payloads. Programs with a screen are titles.
