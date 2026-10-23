#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build out
CC=${CC:-gcc}
command -v "$CC" >/dev/null 2>&1 || { echo "Compilador C nao encontrado: $CC" >&2; exit 1; }
# O find garante que os modulos novos tambem entrem na compilacao.
# shellcheck disable=SC2046
"$CC" -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Isrc $(find src -name '*.c' -print) -o build/minihv
printf '%s\n' 'Build concluido: build/minihv'
