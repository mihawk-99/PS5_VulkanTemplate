# Identity, packaging and the title folder

## The title folder

What the console runs, and what deploying copies to `/data/homebrew/<TITLE_ID>/`:

```text
<TITLE_ID>/
  eboot.bin              the title: the linked ELF, converted and signed as a fake SELF
  sce_module/libc.prx    the clean-room libc module titles load (PS5_Vulkan/runtime/)
  sce_sys/param.json     the identity
  sce_sys/icon0.png      the home-screen icon
  sce_sys/pic0.dds, pic1.dds, snd0.at9    optional backgrounds and sound
  ...                    the program's own files: assets, configs; logs it writes
```

While the title runs, that folder is `/app0/`.

## Building `eboot.bin`

Three steps, all PS5_Vulkan's tools (a title's `ps5/tools/link-title.sh` does them,
called by `ps5/tools/build.sh`):

1. **Link** with `prospero-lld`, PS5_Vulkan's linker script and its link recipe,
   `-e _start`, into a PIE (`build/ps5/link/llvm-pie.elf`). Keep this file: it is
   what crash addresses are symbolised against.
2. **Convert**: `ps5-native-tool link --in llvm-pie.elf --out eboot.elf --stub-dir <sdk>/target/lib --stub <extra stubs> --module-sdk 0x02000009 --companion-sdk 0x08050001 --file-name eboot.elf`.
   This writes the console's dynamic-linking tables. It refuses a title that exports
   symbols: link with `--exclude-libs=ALL` and PS5_Vulkan's `app-symbols.map`.
3. **Sign**: `ps5-native-tool self --sign --in eboot.elf --out eboot.bin --magic 0x1D3D154F`.
   The console loads a fake SELF, not an ELF. An unsigned `eboot.bin` fails to start,
   with no message of its own. `ps5-native-tool self --inspect --file eboot.bin`
   checks the result.

`sce_module/libc.prx` comes from `PS5_Vulkan/runtime/`. Check it against its
`libc.prx.sha256` before copying.

## `param.json`: the one place identity is written

Everything else reads it: the build names the output folder from it, the deploy
tool and the console's loader use it. Validate it in the build.

| Field | Shape |
| --- | --- |
| `titleId` | `PPSA` and five digits; the folder's name |
| `conceptId` | the five digits |
| `contentId` | `UP9000-<titleId>_00-<16 upper-case letters or digits>` |
| `localizedParameters.en-US.titleName` | the name the home screen shows |
| `contentVersion` | `NN.NNN.NNN` |
| `attribute3` | `0x80040` (524352) for 119.88 Hz output, `0` for 59.94 Hz |
| `applicationDrmType` | `free` |
| `kernel.flexibleMemorySize` | optional: the title's flexible memory in bytes, 2097152 to 1073741824 (below) |

The rest is constant across my titles: PS5_VulkanTemplate's `ps5/tools/new-title.py`
writes the whole file (`param_json`); it writes no `kernel` key.

### A larger flexible pool: `kernel.flexibleMemorySize`

A title's flexible memory (what plain anonymous `mmap`, executable mappings of it
included, libc's own heap and shared-memory objects draw from) is 448 MiB unless its
`param.json` asks otherwise. The console reads the request from `param.json` at every launch:

```json
"kernel": {
  "flexibleMemorySize": 1073741824
}
```

What my console showed (firmware 10.01, measured in PS5_Proton, PPSA99170, a title
on this foundation):

- **1 GiB is the most.** At 1073741824 klog reports `FMEM size: 0x40000000` for the
  title and every process it starts, and the title started with 1,037,041,664 bytes
  of flexible memory free instead of 433,061,888. At 2147483648 the launch is
  refused (`0x80020016`, an error dialog on the home screen) and klog states the
  accepted range: 2097152 to 1073741824.
- **It comes out of direct memory.** At 1 GiB, klog's `DMEM size` falls from
  `0x300000000` to `0x2dc000000`: 576 MiB less direct memory, the total unchanged.
- **The executable cannot ask for it.** The memory-parameter block the native tool
  writes into the executable's process parameters had no effect with a size in it,
  whether its pointers were relocated or stored in the file (PS5_Vulkan branch
  `experiment/flexible-memory`).

Leave it out unless the title runs short of flexible memory: the platform layer
already keeps the heap, JIT code and guest memory in direct memory
(`platform-layer.md`), and every title that asks for more gives up direct memory
for it. A title with many processes, or with large shared-memory or executable
mappings, is the case for it; measure free flexible memory before and after
(`sceKernelAvailableFlexibleMemorySize`).

**Title ids in use** (the generator refuses them): PPSA99002 ProsperoLight, 99008
ProsperoEden, 99010 vkQuake, 99014 and 99015 PS5_Vulkan's RADV and CTS titles,
99100 the old template, 99130 Vulkan Template, 99169 RetroArch, 99988 PS5_Vulkan's runner, 99996 to 99999
PS5_Vulkan's canaries and default profile. Check the console's `/data/homebrew/` as
well.

## Assets

- **`icon0.png`**: 512x512 PNG. A title's own artwork, or a generated placeholder
  (`new-title.py` draws one). Never another project's or a game's.
- **`pic0.dds`, `pic1.dds`**: 3840x2160, DDS with a DX10 header, `BC7_UNORM`, one
  level, straight alpha. The header must match the one the console has shown:
  flags `0xA1007` (with the mip count), depth 1, mip count 1, `miscFlags2` 1. A
  header without the mip count and alpha mode was never confirmed to display.
  PS5_RetroArch's `tools/make-title-art.py` draws art and writes exactly that header,
  with its own BC7 encoder.
- **`snd0.at9`**: optional, ATRAC9. Ship none rather than a substitute format.
- **Every asset has a known origin and licence.** Generated by a script in the
  repository, made by me, or under a licence that allows it, written down in the
  title's notices. An image of unrecorded origin is replaced, not shipped
  (`ps5-release` skill).

## The build identity

Hash every input that decides the binary (sources, build scripts, the pinned
revisions, the RADV archive's digest) into one string, compile it in, and print it
first thing at start-up (PS5_RetroArch's `tools/build-title.sh` writes
`build/title_build_identity.h`). A title is deployed once and run many times; the
identity is what ties a klog to the binary that wrote it. Symbolise a crash only
against the build whose identity the run printed.

## Deploying

`python3 ../PS5_Vulkan/tools/deploy-title-folder.py dist/<TITLE_ID>` uploads what
changed and reads every file back (`--always eboot.bin` forces the binary). Two
console behaviours to know:

- **What FTP serves back is not what was uploaded.** A deployed `eboot.bin` reads
  back as a plain ELF of another length: the console keeps its own transformed form
  of an accepted title. Verify the files you built (sizes and digests of
  `dist/`), not the bytes you download.
- **The console's FTP server deviates from the standard.** It answers a successful
  `DELE` with 226 (`ftplib.delete()` raises), resolves paths from the root (use
  absolute paths), and lists inconsistently with a path argument (change directory,
  then list). Use the project tools, which handle all three.

`.env` (ignored) holds `PS5_HOST`, `FTP_PORT` (2121), `KLOG_PORT` (3232) and
`PS5VKCTL_PORT` (9111).
