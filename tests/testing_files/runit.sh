#!/bin/bash

# Author: Hubertus Franke  (frankeh@cims.nyu.edu)
OUTDIR=${1:-.}
shift
SCHED=${*:-../src/sched}
echo "sched=<$SCHED> outdir=<$OUTDIR>"

# if you want -v output  run with ...  ./runit.sh youroutputdir   sched -v 

RFILE=./rfile
INS="0 1 2 3 4 5 6"

# SCHEDS="F L S R2 R5 P2 P5 E2 E4"
SCHEDS="  F    L    S   R2    R5    P2   P5:3  E2:5 E4  E8:9 E1 E2:1 P2 P2:1 P1:2 P15:1 P1:15 P15:14 P2:5 E2:9 E9:2 E15:9 E9:20 E1:1 P1:1 R1 R2 R8 R9 P9:1 F L S2 S1 L1 F1 F9  P2 P4 P8 P10 P50 E3 E8 E9 E20"

for f in ${INS}; do
	for s in ${SCHEDS}; do 
		echo "${SCHED} ${SCHEDARGS} -s${s} input${f} ${RFILE}"
		${SCHED} -s${s} input${f} ${RFILE} > ${OUTDIR}/out_${f}_${s}
	done
done

