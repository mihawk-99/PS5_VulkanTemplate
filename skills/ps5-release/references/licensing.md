# Licensing

## What a title is made of

| Part | Licence |
| --- | --- |
| my titles and frontends (PS5_RetroArch's port code) | GPL-3.0-or-later |
| PS5_VulkanTemplate: the PS5 Vulkan Samples title, and the foundation a title made with its `new-title.py` gets (Sascha Willems' examples and base class, my PS5 layer) | MIT; the built title links the platform layer, so it is distributed under GPL-3.0 |
| the platform layer (`libps5platform.a`) | GPL-3.0-or-later |
| BlackBearReloaded's ps5-native-app-boilerplate and ProsperoLight code | GPL-3.0-or-later |
| RADV and Mesa | MIT, with some parts under other permissive licences (Mesa's own notices) |
| the payload SDK's libc, libc++, libunwind | as released upstream (permissive) |
| dlmalloc, musl's regex | MIT, public domain |

So **every title links GPL-3.0-or-later code statically**. A distributed title is
GPL-3.0-or-later as a whole, ships with its complete corresponding source, and can
only include parts whose licences are compatible with GPL-3.0.

## Compatibility, the cases that matter

| Part's licence | Ship it with a GPL-3.0 title? |
| --- | --- |
| MIT, BSD, zlib, Apache-2.0, MPL-2.0, LGPL-2.1-or-later, GPL-2.0-or-later, GPL-3.0 | yes, with its notices |
| **GPL-2.0-only** (RPCS3) | **no**: it cannot be combined with GPL-3.0 code into one distributed program. Source-only: people build it for their own console |
| non-commercial licences (Snes9x, FinalBurn Neo, Genesis Plus GX) | they ship in PS5_RetroArch as separate cores. The package **may never be sold**; FBNeo's licence also forbids asking for donations for a project that uses its code, so no donation link on the release page |
| no licence, unknown origin | no: replace it |

When a part's status is uncertain, it does not ship until the question is settled by
a decision I make explicitly, or by counsel. Building it for my own console is
unaffected.

## The notices in the title folder

Every built title folder carries:
- `LEGAL.txt` at the top: no piracy (no games, BIOS, firmware or keys, legally
  obtained backups only), RPCS3 source-only, the non-commercial parts;
- `licenses/`: the licence texts each part requires, `components.json` (every part:
  its name, licence, copyright, modifications, the files it produced with their
  sha256, and its source revision and URL), and `README.txt`, which opens with the
  same legal notice.

PS5_RetroArch generates them at build time (`tools/stage-notices.py` from
`tooling/notices/components.json`, checked by `tools/check-notices.py`). A new title
copies that machinery when it starts shipping third-party parts. A minimal title
(one made with `new-title.py`) needs its `LICENSE.md`, the RADV/Mesa and SDK notices,
and its assets' (`ps5/ASSETS.md`; the title folder's `assets/NOTICES.txt`).

Adding a part: one entry in `components.json` (`id`, `name`, `licence`, `texts`,
`copyright`, `modifications`, `artifacts`, `source`), its licence texts under
`tooling/notices/`, and a release check that passes. Every executable file in the
package must be covered by some entry, or the check fails.

## Source

Each release attaches the complete source of everything in the ZIP
(`tools/source-bundle.py`: one archive per part at its pinned revision, with
submodules, plus `SHA256SUMS` and `SOURCES.txt`). That satisfies GPL-3.0 (source
beside the binary) and GPL-2.0's "access from the same place", and it covers parts
fetched from repositories I do not control. A part built from uncommitted source
cannot be released: the notices record `dirty`, and the release check refuses it.

## Trademarks and disclaimers

Every README and release ends with: not affiliated with or endorsed by Sony
Interactive Entertainment; "PlayStation" and "PS5" are trademarks of Sony
Interactive Entertainment. Vulkan is a registered trademark of the Khronos Group, and
the RADV port is not a conformant product (its `conformanceVersion` is 0.0.0.0).
Credit every upstream project and author whose work ships.
