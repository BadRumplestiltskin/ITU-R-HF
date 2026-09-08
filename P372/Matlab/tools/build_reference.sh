#!/bin/bash
# Builds the original ITU P372 C code with a small driver and regenerates tests/reference/.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
# ITU C sources: P372_C_SRC, else ../../Src/P372 (repository layout)
CSRC="${P372_C_SRC:-$ROOT/../Src/P372}"
BUILD="$HERE/build"
mkdir -p "$BUILD" "$ROOT/tests/reference"
cp "$CSRC/Common.h" "$BUILD/common.h"   # Noise.c includes "common.h" (lowercase)
cp "$CSRC/Common.h" "$CSRC/Noise.h" "$BUILD/"
clang -O2 -fcommon -w -include stdlib.h -include errno.h -I"$BUILD" -o "$BUILD/ref_driver" \
    "$HERE/ref_driver.c" "$CSRC/Noise.c" "$CSRC/MakeNoise.c" \
    "$CSRC/NoiseMemory.c" "$CSRC/InitializeNoise.c" -lm
"$BUILD/ref_driver" "$ROOT/Data/" "$ROOT/tests/reference/"
echo "Reference data written to $ROOT/tests/reference/"
ls -la "$ROOT/tests/reference/"
clang -O2 -fcommon -w -include stdlib.h -include errno.h -I"$BUILD" -o "$BUILD/ref_mode2" \
    "$HERE/ref_mode2.c" "$CSRC/Noise.c" "$CSRC/MakeNoise.c" \
    "$CSRC/NoiseMemory.c" "$CSRC/InitializeNoise.c" -lm
"$BUILD/ref_mode2" "$ROOT/Data/" "$ROOT/tests/reference/"
echo "Mode 2 reference CSVs written."
