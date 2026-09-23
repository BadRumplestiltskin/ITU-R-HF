#!/bin/bash
set -e
# The checkout this script lives in.
R=$(cd "$(dirname "$0")/.." && pwd)
cd /tmp/iturhf-cbuild

# Lenient flags + force-include standard headers the old code relied on implicitly.
CFLAGS="-std=gnu11 -w -fPIC -Wno-implicit-function-declaration"
INC="-I$R/Include -include stdio.h -include stdlib.h -include string.h -include errno.h -include math.h -include time.h"

P372=$R/P372/Src/P372
P533=$R/P533/Src/P533
HFP=$R/ITURHFProp/Src/ITURHFProp

echo "== building libp372.so =="
cc $CFLAGS $INC -shared -I"$P372" \
  "$P372/InitializeNoise.c" "$P372/MakeNoise.c" "$P372/Noise.c" "$P372/NoiseMemory.c" \
  -o libp372.so -lm
echo "  ok: $(ls -la libp372.so | awk '{print $5}') bytes"

echo "== building libp533.so =="
cc $CFLAGS $INC -shared -I"$P533" \
  "$P533/Between7000kmand9000km.c" "$P533/CalculateCPParameters.c" "$P533/CircuitReliability.c" \
  "$P533/ELayerScreeningFrequency.c" "$P533/Geometry.c" "$P533/InitializePath.c" "$P533/InputDump.c" \
  "$P533/MUFBasic.c" "$P533/MUFOperational.c" "$P533/MUFVariability.c" "$P533/Magfit.c" \
  "$P533/MedianAvailableReceiverPower.c" "$P533/MedianSkywaveFieldStrengthLong.c" \
  "$P533/MedianSkywaveFieldStrengthShort.c" "$P533/P533.c" "$P533/PathMemory.c" \
  "$P533/ReadIonParameters.c" "$P533/ReadP1239.c" "$P533/ReadType13.c" "$P533/ValidatePath.c" \
  -L. -lp372 -o libp533.so -lm
echo "  ok: $(ls -la libp533.so | awk '{print $5}') bytes"

echo "== building ITURHFProp executable =="
cc $CFLAGS $INC -I"$HFP" \
  "$HFP/DumpPathData.c" "$HFP/ITURHFProp.c" "$HFP/ReadInputConfiguration.c" \
  "$HFP/Report.c" "$HFP/ValidateITURHFP.c" \
  -o ITURHFProp -ldl -lm
echo "  ok: $(ls -la ITURHFProp | awk '{print $5}') bytes"
echo "BUILD DONE"
