#!/usr/bin/env bash
# PS5 Vulkan Template - compile a program's GLSL shaders to the SPIR-V it loads.
#
#   ps5/tools/compile-shaders.sh ID...     shaders/glsl/<id>/*.<stage> -> *.<stage>.spv
#
# As upstream does, the SPIR-V is committed beside its source and the build
# packages it as it is: after changing a shader, run this and commit both.
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: MIT
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
command -v glslangValidator >/dev/null || { echo "glslangValidator was not found" >&2; exit 2; }
(( $# )) || { echo "usage: ${0##*/} ID..." >&2; exit 2; }
for id in "$@"; do
    shopt -s nullglob
    for source in "$root/shaders/glsl/$id/"*.{vert,frag,comp,geom,tesc,tese,mesh,task,rgen,rchit,rmiss,rahit,rint,rcall}; do
        glslangValidator -V --target-env vulkan1.3 "$source" -o "$source.spv" > /dev/null
        echo "$source.spv"
    done
done
