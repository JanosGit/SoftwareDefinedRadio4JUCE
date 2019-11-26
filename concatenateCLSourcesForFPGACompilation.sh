#!/bin/bash

# This script concatenates various .cl sourcefiles found in the module
# to one single file that can then be compiled by the Intel FPGA offline
# CL compiler. It strips away undesired C++ string literals and license
# headers that are found in the single files to include them as strings
# in non-fpga builds.
#
# Pass an absolute path to the output file to generate, followed by the 
# desired cl sourcefiles relative to the ntlab_software_defined_radio
# base folder as arguments to the script.


argcount=1

for clSource in "$@"
do	
    if [ $argcount -eq 1 ]
    then
        outputFile=$clSource;
        rm $outputFile
    else
        head -n -1 "$clSource" | tail -n +18 >> $outputFile
    fi

    argcount=$((argcount + 1))
done

echo "File written to $outputFile"

