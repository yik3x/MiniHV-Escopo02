#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
if [ ! -x build/minihv ]; then ./scripts/build.sh; fi
exec ./build/minihv -c conf/hv.conf "$@"
