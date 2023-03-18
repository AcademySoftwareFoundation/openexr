#!/usr/bin/env bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

set -ex

if [ $# -gt 0 ]
then
   build=$1  # use explicity-provided build directory
   coverage="$build/_coverage"
else
   build='../_build'  # from with CI, use the _build subdirectory
   coverage="_coverage"
fi
   
mkdir -p $coverage
cd $coverage

for gcno in $(find $build -name "*.gcno" -type f); do

    # Identify the original source file (.cpp or .c) from the .gcno
    # file by examining the .o.d make dependency file. The source
    # file should be the second line in the .o.d file.
    
    # gcno = $build/src/bin/exrheader/CMakeFiles/exrheader.dir/main.gcno
    # object_directory = $build/src/bin/exrheader/CMakeFiles/exrheader.dir
    # source_base = $build/src/bin/exrheader/CMakeFiles/exrheader.dir/main
    # dependenty_file = $build/src/bin/exrheader/CMakeFiles/exrheader.dir/main.o.d

    object_directory=$(dirname "$gcno")
    source_base=$(basename "$gcno" ".gcno")
    dependency_file=$object_directory/$source_base.o.d

    if [ -f "$dependency_file" ]; then

        source_file=$(head -2 $dependency_file | tail -1 | sed -e 's/ //' -e 's/ \\//') 
        gcov -l -p -o $object_directory $source_file 
    fi
done



