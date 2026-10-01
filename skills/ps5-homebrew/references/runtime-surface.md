# The runtime surface: symbols that link but do not exist

The commonest way a new piece of code dies on its first console run, and the least
visible from the build.

## The trap

1. The SDK's headers **declare** a function. Compiling succeeds.
2. The link is satisfied by a **stub library** (`libc_stub_weak.so`,
   `libkernel.so` and the other `.so` files in `<sdk>/target/lib`). Stubs exist so
   that a link succeeds, not so that a call works.
3. The console's runtime **does not export** it, or exports it and refuses the
   title, or faults in it. The import slot is never filled, and the call jumps to
   address zero.

Nothing in the build can warn about this. The console reports it as an
instruction-fetch SIGSEGV with `rip: 0`, whose backtrace names the *caller* (a call
through NULL pushes no frame: `ps5-console` skill, crash reading).

The canonical case is `getcwd`: declared, linked, imported harmlessly for months,
and faulting the first time a title called it. Others found the same way:
`arc4random` (libc++'s `random_device` called it through NULL), `access` (exported,
refused for every path), the directory functions, `statvfs`, `getpwuid_r`,
`clock_nanosleep`, `regcomp`, `backtrace`, `dladdr`, the `*_l` locale family.
A function the console does export can still answer wrongly: its `localeconv()`
gives an empty decimal point, so code that builds numbers from it (nlohmann::json,
behind tinygltf) loses every fraction. The platform layer's C-locale `localeconv`
fixes it for titles whose SDK pin has it (fa69d00 or later).

## The defence: the platform layer and the link recipe

Every gap found so far has an implementation in the platform layer
(`ps5platform/libc.h`), and PS5_Vulkan's `tools/radv-link.sh` binds libc's name to it
(`--defsym name=ps5_name`). A title linked with the recipe gets all of them. So:

- **Use the recipe**, unchanged (`vulkan-on-radv.md`).
- **Before calling a libc or POSIX function the console may not have** (files and
  directories, users, locale, time zones, networking, process control), look it up
  in `ps5platform/libc.h` and in the recipe's `--defsym` lists. If it is there, it is
  covered.
- **When a new one faults**, the fix is not a shim in the title: add it to the
  platform layer with a host test, bind it in the recipe, bump the pins
  (`platform-layer.md`). The next title then never meets it.

## Recognising one on the console

- SIGSEGV, `rip` 0 or wild, fault address equal to `rip`: a call through an unfilled
  import. Look at what the frame named by the backtrace *calls*.
- SIGSYS with `rip` in libkernel's range (`0x8xxxxxxxx`): a system call the
  console refuses to a title. That is not your code's bug; route the call through
  the platform layer, or avoid it.
- A function that returns an error for every input (`access` → EACCES): exported
  and refused. Same fix.

## The older check tool

PS5_vkQuake and PS5_RetroArch carry `tools/check-runtime-surface.py`. It compares a
linked ELF's undefined symbols with the clean-room `libc.prx`'s export list. It was
written for the ps5vk link, and on a title that links the RADV archive it lists
thousands of Mesa's own unresolved internal symbols. Use it only on code linked
without RADV (a library's test program, a payload), where its two tiers still read
correctly: "nothing provides it" means a certain fault, and "only a link stub
provides it" means unproven until a console run calls it.

## The rule

**Find them by checking, never by guessing**, and fix each one once, in the
platform layer, for every title.
