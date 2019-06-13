#!/bin/bash

# This script concatenates various .cl sourcefiles found in the module
# to one single file that can then be compiled by the Intel FPGA offline
# CL compiler. It strips away undesired C++ string literals and license
# headers that are found in the single files to include them as strings
# in non-fpga builds.
#
# Pass the desired paths of the cl sourcefiles relative to the 
# ntlab_software_defined_radio base folder as arguments to the script.
# The resulting output file concatenatedCLSources.cl will be placed in
# this folder

outputFile="`dirname "$0"`/concatenatedCLSources.cl"
sourceBaseDir="`dirname "$0"`/ntlab_software_defined_radio"

rm $outputFile

for clSource in "$@"
do	
    head -n -1 "$sourceBaseDir/$clSource" | tail -n +18 >> $outputFile
done

