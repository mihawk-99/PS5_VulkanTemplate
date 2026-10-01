#!/usr/bin/env bash
# PS5 Vulkan Template - set up everything a title builds against, beside this repository.
#
#   ps5/tools/bootstrap.sh            check the host, clone what is missing, build what is missing
#   ps5/tools/bootstrap.sh --check    only report what is there and what is not
#   ps5/tools/bootstrap.sh --rebuild-radv
#                                     also rebuild RADV when its archive is not at the
#                                     revision PS5_Vulkan pins (a long build)
#
# A title links RADV, Mesa's Vulkan driver, built for the console by PS5_Vulkan
# from PS5_Mesa, and the platform layer of the payload SDK fork, PS5_PayloadSDK.
# They are checked out beside this repository (../PS5_Vulkan, ../PS5_Mesa,
# ../PS5_PayloadSDK) and built once:
#
#   PS5_Vulkan   tools/setup-native-dependencies.sh   its payload SDK, at its pin
#                make                                  the native tool and libc.prx
#                tools/build-radv.sh release           the RADV release archive
#                tools/build-ps5vkctl.sh               the console's control payload
#   this one     git submodule update --init external/glm   glm (the asset pack is not needed)
#                ps5/tools/setup-sdk.sh                this repository's SDK pin
#
# What it never does: rebuild a RADV archive that is already there (unless
# asked), touch the console, or write the console's address (PS5_Vulkan's .env,
# which every console tool reads).
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: MIT
set -euo pipefail

ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
parent=$(dirname "$root")
check=false
rebuild_radv=false
for argument in "$@"; do
    case $argument in
        --check) check=true ;;
        --rebuild-radv) rebuild_radv=true ;;
        *) echo "usage: ${0##*/} [--check] [--rebuild-radv]" >&2; exit 2 ;;
    esac
done
vulkan=${PS5_VULKAN_DIR:-$parent/PS5_Vulkan}
mesa=${PS5_MESA_FORK:-$parent/PS5_Mesa}
sdk_fork=${PS5_PAYLOAD_SDK_FORK:-$parent/PS5_PayloadSDK}
problems=0
ok() { printf '  ok      %s\n' "$*"; }
todo() { printf '  missing %s\n' "$*"; problems=$((problems + 1)); }
step() { printf '==> %s\n' "$*"; }

step "the host"
for command in git cmake clang python3 glslangValidator rsync curl zstd make; do
    command -v "$command" > /dev/null && ok "$command" || todo "$command (install it with the system's packages)"
done
for command in ninja meson; do
    command -v "$command" > /dev/null || [[ -x $HOME/.local/bin/$command ]] && ok "$command" ||
        todo "$command (uv tool install $command, or the system's package)"
done
for module in numpy PIL mako; do
    python3 -c "import $module" 2> /dev/null && ok "python: $module" ||
        todo "python: $module (python3 -m pip install --user ${module/PIL/Pillow})"
done
clang_major=$(clang --version 2> /dev/null | sed -n 's/.*clang version \([0-9]*\).*/\1/p' | head -1)
[[ -n $clang_major && $clang_major -ge 18 ]] && ok "clang $clang_major (18 or later)" || todo "clang 18 or later"

step "the projects beside this one"
for pair in "PS5_Vulkan $vulkan" "PS5_Mesa $mesa" "PS5_PayloadSDK $sdk_fork"; do
    read -r name path <<< "$pair"
    if [[ -d $path/.git ]]; then
        ok "$name ($path, $(git -C "$path" rev-parse --short HEAD))"
    elif $check; then
        todo "$name: git clone https://github.com/mihawk-99/$name.git $path"
    else
        echo "  cloning $name into $path"
        git clone -q "https://github.com/mihawk-99/$name.git" "$path"
        ok "$name"
    fi
done

step "PS5_Vulkan's build"
if [[ -d $vulkan ]]; then
    sdk_pin=$(sed -n 's/^sdk_revision=//p' "$vulkan/tools/setup-native-dependencies.sh")
    if [[ -f $vulkan/.deps/native/ps5-payload-sdk/.ps5-sdk-revision &&
            $(< "$vulkan/.deps/native/ps5-payload-sdk/.ps5-sdk-revision") == "$sdk_pin" ]]; then
        ok "its payload SDK, at ${sdk_pin:0:12}"
    elif $check; then
        todo "its payload SDK: (cd $vulkan && tools/setup-native-dependencies.sh)"
    else
        (cd "$vulkan" && bash tools/setup-native-dependencies.sh > /dev/null) && ok "its payload SDK, at ${sdk_pin:0:12}"
    fi
    if [[ -x $vulkan/build/host/ps5-native-tool && -f $vulkan/runtime/libc.prx ]]; then
        ok "the native tool and libc.prx"
    elif $check; then
        todo "the native tool and libc.prx: (cd $vulkan && make)"
    else
        # make builds the native tool and libc.prx first, then PS5_Vulkan's own Hello
        # World, which a title does not need: what counts is the first two
        (cd "$vulkan" && make > "$vulkan/build-bootstrap-make.log" 2>&1) || true
        [[ -x $vulkan/build/host/ps5-native-tool && -f $vulkan/runtime/libc.prx ]] && ok "the native tool and libc.prx" ||
            { echo "  make did not build them: $vulkan/build-bootstrap-make.log" >&2; exit 1; }
    fi
    mesa_pin=$(sed -n 's/^mesa_revision=//p' "$vulkan/tools/build-radv.sh")
    provenance="$vulkan/.deps/native/radv-release/PROVENANCE.txt"
    built=$(sed -n 's/^revision: //p' "$provenance" 2> /dev/null || true)
    if [[ -n $built && $built == "$mesa_pin" ]]; then
        ok "the RADV release archive, at PS5 Mesa ${built:0:11} (PS5_Vulkan's pin)"
    elif [[ -n $built ]] && ! $rebuild_radv; then
        ok "the RADV release archive, at PS5 Mesa ${built:0:11}; PS5_Vulkan pins ${mesa_pin:0:11}: kept (--rebuild-radv rebuilds it)"
    elif $check; then
        todo "the RADV release archive: (cd $vulkan && tools/build-radv.sh release), a long build"
    else
        echo "  building RADV at PS5 Mesa ${mesa_pin:0:11} (a long build)"
        (cd "$vulkan" && bash tools/build-radv.sh release) && ok "the RADV release archive"
    fi
    if [[ -f $vulkan/build/ps5vkctl/ps5vkctl.elf ]]; then
        ok "the control payload, ps5vkctl"
    elif $check; then
        todo "the control payload: (cd $vulkan && tools/build-ps5vkctl.sh)"
    else
        (cd "$vulkan" && bash tools/build-ps5vkctl.sh > /dev/null) && ok "the control payload, ps5vkctl"
    fi
    if [[ -f $vulkan/.env ]] && grep -q "^PS5_HOST=" "$vulkan/.env"; then
        ok "the console's address (PS5_Vulkan's .env)"
    else
        todo "the console's address: PS5_HOST=<address> in $vulkan/.env (FTP_PORT=2121, KLOG_PORT=3232, PS5VKCTL_PORT=9111 by default)"
    fi
fi

step "this repository"
if [[ -f $root/external/glm/glm/glm.hpp ]]; then
    ok "glm (a submodule)"
elif $check; then
    todo "glm: git -C $root submodule update --init external/glm"
else
    git -C "$root" submodule update --init -q external/glm && ok "glm (a submodule)"
fi
sdk_revision=$(sed -n 's/^sdk_revision=//p' "$ps5/tools/setup-sdk.sh")
if [[ -f $root/.deps/native/ps5-payload-sdk/.ps5-sdk-revision &&
        $(< "$root/.deps/native/ps5-payload-sdk/.ps5-sdk-revision") == "$sdk_revision" ]]; then
    ok "its payload SDK, at ${sdk_revision:0:12}"
elif $check; then
    todo "its payload SDK: ps5/tools/setup-sdk.sh"
else
    bash "$ps5/tools/setup-sdk.sh" > /dev/null && ok "its payload SDK, at ${sdk_revision:0:12}"
fi

echo
if (( problems )); then
    echo "$problems to do (above)."
    exit 1
fi
cat << 'EOF'
Ready. On the console (once per boot, with its homebrew environment running: an
enabler such as etaHEN, ShadowMountPlus, ftpsrv on 2121 and klogsrv on 3232), load
the control payload from PS5_Vulkan:
    python3 tools/ps5_console.py deploy-payload, then load it with the console's payload loader
Then: ps5/tools/build.sh && ps5/tools/deploy.sh && ps5/tools/run.sh
EOF
