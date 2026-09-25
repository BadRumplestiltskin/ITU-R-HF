#!/bin/sh
# Shared by the read-* cases (this directory has no cmd, so cases.sh skips it).
#
# mkin.sh [KEY=value ...] - writes an ITURHFProp .in file to stdout: one
# 175 km circuit (Luxembourg - Bockhacken, as ITURHFProp/Bin/1-5-85.in), one
# month, hour and frequency. Each KEY=value overrides a default below
# (MMN is written as given, so a category needs its quotes: MMN='"CITY"').
DATA=$P533DATA/
TXANT=ISOTROPIC
RXANT=ISOTROPIC
ORIENT=TX2RX
TXB=0.0
RXB=0.0
MONTH=5
HOUR=20
FREQ=6.1
RPT="RPT_GRW | RPT_SNR"
TXLAT=49.6666666667
TXLNG=6.31666666667
YEAR=1985
SSN=18
TXPOWER=0.0
BW=1000.0
SNRR=10.0
SNRXXP=10
MMN='"RURAL"'
MOD=ANALOG
SIRR=23.76
A=0.0
TW=0.0
FW=0.0
T0=0.0
F0=0.0
for a; do
	k=${a%%=*}
	v=${a#*=}
	eval "$k=\$v"
done
cat <<END
PathName "read case"
PathTXName "LUXEMBURG"
Path.L_tx.lat $TXLAT
Path.L_tx.lng $TXLNG
TXAntFilePath "$TXANT"
TXGOS 0.0
TXBearing $TXB
PathRXName "BOCKHACKEN"
Path.L_rx.lat 51.1166666667
Path.L_rx.lng 7.26666666667
RXAntFilePath "$RXANT"
RXGOS 0.0
RXBearing $RXB
AntennaOrientation "$ORIENT"
Path.year $YEAR
Path.month $MONTH
Path.hour $HOUR
Path.SSN $SSN
Path.frequency $FREQ
Path.txpower $TXPOWER
Path.BW $BW
Path.SNRr $SNRR
Path.SNRXXp $SNRXXP
Path.ManMadeNoise $MMN
Path.Modulation "$MOD"
Path.SIRr $SIRR
Path.A $A
Path.TW $TW
Path.FW $FW
Path.T0 $T0
Path.F0 $F0
Path.SorL "SHORTPATH"
RptFileFormat "$RPT"
LL.lat 51.1166666667
LL.lng 7.26666666667
LR.lat 51.1166666667
LR.lng 7.26666666667
UL.lat 51.1166666667
UL.lng 7.26666666667
UR.lat 51.1166666667
UR.lng 7.26666666667
latinc 1.0
lnginc 1.0
DataFilePath "$DATA"
END
