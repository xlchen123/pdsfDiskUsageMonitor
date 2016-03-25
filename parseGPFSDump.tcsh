#!/bin/tcsh
#
# Author: Jochen Thaeder (LBNL) <jochen@thaeder.de>
# Run script to produce root outfile of GPFS Dump
#
###################################################

set BASEPATH=$1
set runMode=$2

pushd ${BASEPATH} > /dev/null

if ( $runMode == 0 ) then
    root -b -l -q parseGPFSDump.C++'(0,0)'
    root -b -l -q parseGPFSDump.C++'(0,1)'
endif

if ( $runMode == 1 ) then
    root -b -l -q parseGPFSDump.C++'(1)'
endif

if ( $runMode == 2 ) then
    root -b -l -q parseGPFSDump.C+'(0,0)'
    root -b -l -q parseGPFSDump.C+'(0,1)'
    root -b -l -q parseGPFSDump.C+'(1)'
endif

popd > /dev/null


